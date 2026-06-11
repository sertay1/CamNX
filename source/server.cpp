#include "server.hpp"
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <switch.h>
#include <vector>
#include <algorithm>
#include "qrcodegen.h"

#define PHOTOS_DIR "/switch/CamNX/photos/"

Server::Server(SDL_Renderer* ren)
    : renderer(ren), running(false), threadStarted(false), serverSocket(-1), qrTexture(nullptr) {
    ipAddress = getLocalIP();
}

Server::~Server() {
    stop();
    if (qrTexture) SDL_DestroyTexture(qrTexture);
}

std::string Server::getLocalIP() {
    u32 ip = 0;
    if (R_SUCCEEDED(nifmGetCurrentIpAddress(&ip))) {
        struct in_addr addr;
        addr.s_addr = ip;
        return std::string(inet_ntoa(addr));
    }
    return "0.0.0.0";
}

void Server::generateQR() {
    if (ipAddress == "0.0.0.0" || ipAddress.empty()) return;
    std::string url = "http://" + ipAddress + ":8080";
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    bool ok = qrcodegen_encodeText(url.c_str(), tempBuffer, qrcode, qrcodegen_Ecc_LOW,
                                   qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
                                   qrcodegen_Mask_AUTO, true);
    if (!ok) return;
    int size = qrcodegen_getSize(qrcode);
    int scale = 8;
    int texSize = size * scale;
    SDL_Surface* surface = SDL_CreateRGBSurface(0, texSize, texSize, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!surface) return;
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 255, 255));
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (qrcodegen_getModule(qrcode, x, y)) {
                SDL_Rect rect = {x * scale, y * scale, scale, scale};
                SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, 0, 0, 0));
            }
        }
    }
    qrTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
}

void Server::refreshIP() {
    ipAddress = getLocalIP();
    if (qrTexture) { SDL_DestroyTexture(qrTexture); qrTexture = nullptr; }
    generateQR();
}

void Server::start() {
    // Always do a clean stop first before starting
    stop();
    refreshIP();
    running = true;
    threadStarted = true;
    if (pthread_create(&thread, NULL, threadFunc, this) != 0) {
        running = false;
        threadStarted = false;
    }
}

void Server::stop() {
    if (!running && !threadStarted) return;
    running = false;
    // Close server socket to unblock accept() immediately
    if (serverSocket >= 0) {
        shutdown(serverSocket, SHUT_RDWR);
        close(serverSocket);
        serverSocket = -1;
    }
    if (threadStarted) {
        pthread_join(thread, NULL);
        threadStarted = false;
    }
}

void* Server::threadFunc(void* arg) {
    Server* s = (Server*)arg;
    s->run();
    s->running = false;
    s->threadStarted = false;
    return NULL;
}


void Server::run() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) { running = false; return; }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // SO_LINGER l_onoff=1 l_linger=0 → RST instead of FIN, port freed immediately
    struct linger lg = {1, 0};
    setsockopt(serverSocket, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family      = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port        = htons(8080);

    // Retry bind — port may briefly be in TIME_WAIT after a stop/start
    bool bound = false;
    for (int attempt = 0; attempt < 10 && running; attempt++) {
        if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0) {
            bound = true; break;
        }
        usleep(200000); // 200 ms
    }
    if (!bound) {
        close(serverSocket); serverSocket = -1;
        running = false; return;
    }

    listen(serverSocket, 32);

    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        if (serverSocket < 0) break;
        FD_SET(serverSocket, &readfds);

        struct timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = 100000; // 100 ms

        int activity = select(serverSocket + 1, &readfds, NULL, NULL, &tv);
        if (!running) break;       // stop() was called
        if (activity < 0) break;   // socket forcibly closed
        if (activity == 0) continue;
        if (!FD_ISSET(serverSocket, &readfds)) continue;

        struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
        if (clientSocket < 0) {
            if (!running) break;    // intentional stop
            usleep(10000);          // brief pause on transient error, then retry
            continue;
        }

        // Handle client synchronously — no thread spawning (like NXShare)
        // This prevents thread limit crashes on Switch
        struct timeval tv_rcv = {5, 0};
        struct timeval tv_snd = {30, 0};
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &tv_rcv, sizeof tv_rcv);
        setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, &tv_snd, sizeof tv_snd);
        serveClient(clientSocket);
        close(clientSocket);
    }

    if (serverSocket >= 0) {
        close(serverSocket);
        serverSocket = -1;
    }
}

// ── Helpers ──────────────────────────────────────────────────────────────────
static bool safe_write(int fd, const void* buf, size_t len) {
    const char* p = (const char*)buf;
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = write(fd, p + sent, len - sent);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

static std::string readFullRequest(int clientSocket) {
    std::string req;
    char tmp[4096];
    while (true) {
        ssize_t n = recv(clientSocket, tmp, sizeof(tmp), 0);
        if (n <= 0) break;
        req.append(tmp, n);
        // HTTP request ends with \r\n\r\n
        if (req.find("\r\n\r\n") != std::string::npos) break;
        if (req.size() > 16384) break; // safety cap
    }
    return req;
}

static std::string urlDecode(const std::string& src) {
    std::string result;
    for (size_t i = 0; i < src.size(); i++) {
        if (src[i] == '%' && i + 2 < src.size()) {
            int val = 0;
            sscanf(src.substr(i+1,2).c_str(), "%x", &val);
            result += (char)val;
            i += 2;
        } else if (src[i] == '+') {
            result += ' ';
        } else {
            result += src[i];
        }
    }
    return result;
}

static void send404(int fd) {
    const char* r = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\nConnection: close\r\n\r\nNot Found";
    safe_write(fd, r, strlen(r));
}

// ── Request router ───────────────────────────────────────────────────────────
void Server::serveClient(int clientSocket) {
    std::string request = readFullRequest(clientSocket);
    if (request.empty()) return;

    // Parse method + path
    size_t sp1 = request.find(' ');
    if (sp1 == std::string::npos) return;
    size_t sp2 = request.find(' ', sp1 + 1);
    if (sp2 == std::string::npos) return;
    std::string method = request.substr(0, sp1);
    std::string rawPath = request.substr(sp1 + 1, sp2 - sp1 - 1);

    // Split path and query string
    std::string requestPath, queryStr;
    size_t qmark = rawPath.find('?');
    if (qmark != std::string::npos) {
        requestPath = rawPath.substr(0, qmark);
        queryStr    = rawPath.substr(qmark + 1);
    } else {
        requestPath = rawPath;
    }

    // ── / → serve index.html
    if (requestPath == "/") {
        FILE* fTpl = fopen("romfs:/index.html", "rb");
        if (!fTpl) { send404(clientSocket); return; }
        fseek(fTpl, 0, SEEK_END);
        size_t sz = ftell(fTpl);
        fseek(fTpl, 0, SEEK_SET);
        char* buf = new(std::nothrow) char[sz + 1];
        if (!buf) { fclose(fTpl); send404(clientSocket); return; }
        fread(buf, 1, sz, fTpl);
        buf[sz] = '\0';
        fclose(fTpl);

        std::string hdr  = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n";
                    hdr += "Cache-Control: no-cache\r\n";
                    hdr += "Content-Length: " + std::to_string(sz) + "\r\nConnection: close\r\n\r\n";
        safe_write(clientSocket, hdr.c_str(), hdr.size());
        safe_write(clientSocket, buf, sz);
        delete[] buf;
        return;
    }

    // ── /api/list → JSON file list (no per-file fopen for sizes)
    if (requestPath == "/api/list") {
        int offset = 0, limit = 60;
        std::string filter;
        auto getParam = [&](const std::string& key) -> std::string {
            size_t pos = queryStr.find(key + "=");
            if (pos == std::string::npos) return "";
            size_t start = pos + key.size() + 1;
            size_t end   = queryStr.find('&', start);
            return urlDecode(end == std::string::npos ? queryStr.substr(start) : queryStr.substr(start, end - start));
        };
        std::string offStr = getParam("offset");
        std::string limStr = getParam("limit");
        if (!offStr.empty()) offset = std::stoi(offStr);
        if (!limStr.empty()) limit  = std::stoi(limStr);
        filter = getParam("filter");

        std::vector<std::string> allPhotos, allVideos;
        DIR* d = opendir(PHOTOS_DIR);
        if (d) {
            struct dirent* de;
            while ((de = readdir(d)) != NULL) {
                std::string name = de->d_name;
                if (name.find(".jpg") != std::string::npos || name.find(".png") != std::string::npos)
                    allPhotos.push_back(name);
                else if (name.find(".avi") != std::string::npos)
                    allVideos.push_back(name);
            }
            closedir(d);
        }
        std::sort(allPhotos.begin(), allPhotos.end(), std::greater<std::string>());
        std::sort(allVideos.begin(), allVideos.end(), std::greater<std::string>());

        std::vector<std::string> list;
        bool wantPhotos = filter.empty() || filter == "photos";
        bool wantVideos = filter.empty() || filter == "videos";
        if (wantPhotos) for (auto& f : allPhotos) list.push_back(f);
        if (wantVideos) for (auto& f : allVideos) list.push_back(f);

        int end = std::min(offset + limit, (int)list.size());

        std::string json = "{";
        json += "\"total\":"       + std::to_string(list.size())      + ",";
        json += "\"screenshots\":" + std::to_string(allPhotos.size()) + ",";
        json += "\"videos\":"      + std::to_string(allVideos.size()) + ",";
        json += "\"games\":[\"CamNX Photos\"],";
        json += "\"files\":[";

        for (int i = offset; i < end; i++) {
            const std::string& file = list[i];
            std::string type    = (file.find(".avi") != std::string::npos) ? "video" : "photo";
            std::string dateStr = "2026-06-11", timeStr = "00:00";
            if (file.size() >= 14) {
                dateStr = file.substr(0,4) + "-" + file.substr(4,2) + "-" + file.substr(6,2);
                timeStr = file.substr(8,2) + ":" + file.substr(10,2);
            }
            if (i > offset) json += ",";
            json += "{";
            json += "\"filename\":\"" + file + "\",";
            json += "\"type\":\""     + type + "\",";
            json += "\"date\":\""     + dateStr + "\",";
            json += "\"time\":\""     + timeStr + "\",";
            json += "\"size\":0}";  // skip per-file size fopen — speeds up list dramatically
        }
        json += "]}";

        std::string hdr  = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n";
                    hdr += "Cache-Control: no-cache\r\n";
                    hdr += "Content-Length: " + std::to_string(json.size()) + "\r\nConnection: close\r\n\r\n";
        safe_write(clientSocket, hdr.c_str(), hdr.size());
        safe_write(clientSocket, json.c_str(), json.size());
        return;
    }

    // ── /api/refresh → ack only (actual refresh is done by X button on Switch)
    if (requestPath == "/api/refresh") {
        const char* r = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 15\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}";
        safe_write(clientSocket, r, strlen(r));
        return;
    }

    // ── /logo.png
    if (requestPath == "/logo.png") {
        FILE* f = fopen("romfs:/camnx_uygulama_ici.png", "rb");
        if (!f) { send404(clientSocket); return; }
        fseek(f, 0, SEEK_END);
        size_t sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* buf = new(std::nothrow) char[sz];
        if (!buf) { fclose(f); send404(clientSocket); return; }
        fread(buf, 1, sz, f);
        fclose(f);
        std::string hdr = "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\n";
                    hdr += "Cache-Control: max-age=3600\r\n";
                    hdr += "Content-Length: " + std::to_string(sz) + "\r\nConnection: close\r\n\r\n";
        safe_write(clientSocket, hdr.c_str(), hdr.size());
        safe_write(clientSocket, buf, sz);
        delete[] buf;
        return;
    }

    // ── /photos/<filename> → serve file with Range support
    if (requestPath.size() > 8 && requestPath.substr(0, 8) == "/photos/") {
        std::string filename = urlDecode(requestPath.substr(8));
        // Basic path traversal guard
        if (filename.find("..") != std::string::npos) { send404(clientSocket); return; }

        std::string filepath = std::string(PHOTOS_DIR) + filename;
        FILE* f = fopen(filepath.c_str(), "rb");
        if (!f) { send404(clientSocket); return; }

        fseek(f, 0, SEEK_END);
        long fileSize = ftell(f);
        fseek(f, 0, SEEK_SET);

        std::string contentType = "image/jpeg";
        if (filename.size() >= 4) {
            std::string ext = filename.substr(filename.size() - 4);
            if (ext == ".png")  contentType = "image/png";
            if (ext == ".avi")  contentType = "application/octet-stream"; // force download
        }

        // Check for Range header
        long rangeStart = 0, rangeEnd = fileSize - 1;
        bool hasRange = false;
        size_t rpos = request.find("Range: bytes=");
        if (rpos != std::string::npos) {
            hasRange = true;
            size_t dash = request.find('-', rpos + 13);
            std::string startStr = request.substr(rpos + 13, dash - rpos - 13);
            std::string endStr;
            size_t eol = request.find('\r', dash);
            if (eol != std::string::npos) endStr = request.substr(dash + 1, eol - dash - 1);
            if (!startStr.empty()) rangeStart = std::stol(startStr);
            if (!endStr.empty())   rangeEnd   = std::stol(endStr);
            if (rangeEnd >= fileSize) rangeEnd = fileSize - 1;
        }

        long rangeLen = rangeEnd - rangeStart + 1;
        fseek(f, rangeStart, SEEK_SET);

        std::string hdr;
        if (hasRange) {
            hdr  = "HTTP/1.1 206 Partial Content\r\n";
            hdr += "Content-Range: bytes " + std::to_string(rangeStart) + "-" + std::to_string(rangeEnd) + "/" + std::to_string(fileSize) + "\r\n";
        } else {
            hdr  = "HTTP/1.1 200 OK\r\n";
        }
        hdr += "Content-Type: "   + contentType + "\r\n";
        hdr += "Content-Length: " + std::to_string(rangeLen) + "\r\n";
        if (contentType != "image/jpeg" && contentType != "image/png") {
            hdr += "Content-Disposition: attachment; filename=\"" + filename + "\"\r\n";
        }
        hdr += "Accept-Ranges: bytes\r\n";
        hdr += "Connection: close\r\n\r\n";
        safe_write(clientSocket, hdr.c_str(), hdr.size());

        const size_t CHUNK = 32768; // 32 KB chunks
        char* buf = new(std::nothrow) char[CHUNK];
        if (buf) {
            long remaining = rangeLen;
            while (remaining > 0) {
                size_t toRead = (size_t)std::min((long)CHUNK, remaining);
                size_t got = fread(buf, 1, toRead, f);
                if (got == 0) break;
                if (!safe_write(clientSocket, buf, got)) break;
                remaining -= got;
            }
            delete[] buf;
        }
        fclose(f);
        return;
    }

    send404(clientSocket);
}

void Server::renderQR(int cx, int cy) {
    if (!qrTexture) return;
    int w, h;
    SDL_QueryTexture(qrTexture, NULL, NULL, &w, &h);
    int x = cx - w / 2;
    int y = cy - h / 2;
    SDL_Rect bg = {x - 10, y - 10, w + 20, h + 20};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &bg);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(renderer, qrTexture, NULL, &dst);
}
