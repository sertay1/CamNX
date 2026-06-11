#include "avi_player.hpp"
#include <string.h>

AviPlayer::AviPlayer() {
}

AviPlayer::~AviPlayer() {
    close();
}

bool AviPlayer::load(const std::string& path) {
    close();
    
    file = fopen(path.c_str(), "rb");
    if (!file) return false;

    // Check RIFF AVI header
    char magic[12];
    if (fread(magic, 1, 12, file) != 12) { close(); return false; }
    if (strncmp(magic, "RIFF", 4) != 0 || strncmp(magic + 8, "AVI ", 4) != 0) {
        close();
        return false;
    }

    // Find the 'movi' LIST chunk
    while (!feof(file)) {
        char chunkId[4];
        uint32_t chunkSize;
        if (fread(chunkId, 1, 4, file) != 4) break;
        if (fread(&chunkSize, 1, 4, file) != 4) break;

        uint32_t paddedSize = chunkSize + (chunkSize % 2);

        if (strncmp(chunkId, "LIST", 4) == 0) {
            char listType[4];
            if (fread(listType, 1, 4, file) != 4) break;
            if (strncmp(listType, "movi", 4) == 0) {
                moviStart = ftell(file);
                moviEnd = moviStart + chunkSize - 4;
                playing = true;
                lastFrameTime = 0;
                return true;
            } else {
                fseek(file, paddedSize - 4, SEEK_CUR);
            }
        } else {
            fseek(file, paddedSize, SEEK_CUR);
        }
    }

    close();
    return false;
}

void AviPlayer::close() {
    playing = false;
    if (file) {
        fclose(file);
        file = nullptr;
    }
    if (currentTexture) {
        SDL_DestroyTexture(currentTexture);
        currentTexture = nullptr;
    }
    moviStart = 0;
    moviEnd = 0;
}

void AviPlayer::rewind() {
    if (file && moviStart > 0) {
        fseek(file, moviStart, SEEK_SET);
    }
}

void AviPlayer::update(SDL_Renderer* renderer) {
    if (!file || !playing) return;

    uint32_t now = SDL_GetTicks();
    // 15 FPS = 66ms per frame
    if (now - lastFrameTime < 66 && currentTexture != nullptr) {
        return; // wait for next frame
    }
    lastFrameTime = now;

    if (ftell(file) >= moviEnd) {
        rewind(); // Loop video
    }

    // Try to read until we successfully get a 00dc chunk
    while (ftell(file) < moviEnd) {
        char chunkId[4];
        uint32_t chunkSize;
        if (fread(chunkId, 1, 4, file) != 4) { rewind(); return; }
        if (fread(&chunkSize, 1, 4, file) != 4) { rewind(); return; }

        uint32_t paddedSize = chunkSize + (chunkSize % 2);

        if (strncmp(chunkId, "00dc", 4) == 0) {
            if (chunkSize > 5 * 1024 * 1024) { rewind(); return; } // prevent bad_alloc on corruption
            char* buf = new (std::nothrow) char[chunkSize];
            if (!buf) { rewind(); return; }

            if (fread(buf, 1, chunkSize, file) == chunkSize) {
                SDL_RWops* rw = SDL_RWFromMem(buf, chunkSize);
                SDL_Surface* surf = IMG_Load_RW(rw, 1);
                delete[] buf;
                
                if (paddedSize > chunkSize) {
                    fseek(file, paddedSize - chunkSize, SEEK_CUR);
                }

                if (surf) {
                    if (currentTexture) SDL_DestroyTexture(currentTexture);
                    currentTexture = SDL_CreateTextureFromSurface(renderer, surf);
                    SDL_FreeSurface(surf);
                    return; // Successfully read a frame
                }
            } else {
                delete[] buf;
            }
        } else {
            fseek(file, paddedSize, SEEK_CUR);
        }
    }
    
    // If we reach here, we hit the end of the movi chunk without finding a frame
    rewind();
}

SDL_Surface* AviPlayer::extractFirstFrame(const std::string& path) {
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) return nullptr;

    char magic[12];
    if (fread(magic, 1, 12, file) != 12 || strncmp(magic, "RIFF", 4) != 0 || strncmp(magic + 8, "AVI ", 4) != 0) {
        fclose(file);
        return nullptr;
    }

    // Fast-forward to 'movi' list
    long moviEnd = 0;
    while (!feof(file)) {
        char chunkId[4];
        uint32_t chunkSize;
        if (fread(chunkId, 1, 4, file) != 4) break;
        if (fread(&chunkSize, 1, 4, file) != 4) break;
        uint32_t paddedSize = chunkSize + (chunkSize % 2);

        if (strncmp(chunkId, "LIST", 4) == 0) {
            char listType[4];
            if (fread(listType, 1, 4, file) != 4) break;
            if (strncmp(listType, "movi", 4) == 0) {
                moviEnd = ftell(file) + chunkSize - 4;
                break;
            } else {
                fseek(file, paddedSize - 4, SEEK_CUR);
            }
        } else {
            fseek(file, paddedSize, SEEK_CUR);
        }
    }

    if (moviEnd == 0) { fclose(file); return nullptr; }

    // Read the first 00dc chunk
    while (ftell(file) < moviEnd) {
        char chunkId[4];
        uint32_t chunkSize;
        if (fread(chunkId, 1, 4, file) != 4) break;
        if (fread(&chunkSize, 1, 4, file) != 4) break;
        uint32_t paddedSize = chunkSize + (chunkSize % 2);

        if (strncmp(chunkId, "00dc", 4) == 0) {
            if (chunkSize > 5 * 1024 * 1024) break; // prevent bad_alloc
            char* buf = new (std::nothrow) char[chunkSize];
            if (!buf) break;

            if (fread(buf, 1, chunkSize, file) == chunkSize) {
                SDL_RWops* rw = SDL_RWFromMem(buf, chunkSize);
                SDL_Surface* surf = IMG_Load_RW(rw, 1);
                delete[] buf;
                fclose(file);
                return surf;
            }
            delete[] buf;
        } else {
            fseek(file, paddedSize, SEEK_CUR);
        }
    }

    fclose(file);
    return nullptr;
}
