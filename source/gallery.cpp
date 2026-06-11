#include "gallery.hpp"
#include "i18n.hpp"
#include <SDL2/SDL_image.h>
#include <jpeglib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>
#include <switch.h>

#define PHOTOS_DIR "/switch/CamNX/photos/"

Gallery::Gallery(SDL_Renderer* ren, TTF_Font* font) : renderer(ren), uiFont(font), selectedIndex(0), fullscreen(false) {
    mkdir("/switch", 0777);
    mkdir("/switch/CamNX", 0777);
    mkdir(PHOTOS_DIR, 0777);
    
    // Copy sample photo if it doesn't exist
    FILE* check = fopen("/switch/CamNX/photos/SertAy.jpg", "rb");
    if (!check) {
        FILE* src = fopen("romfs:/SertAy.jpg", "rb");
        if (src) {
            FILE* dst = fopen("/switch/CamNX/photos/SertAy.jpg", "wb");
            if (dst) {
                char buf[8192];
                size_t bytes;
                while ((bytes = fread(buf, 1, sizeof(buf), src)) > 0) {
                    fwrite(buf, 1, bytes, dst);
                }
                fclose(dst);
            }
            fclose(src);
        }
    } else {
        fclose(check);
    }
    
    scanPhotos();
}

Gallery::~Gallery() {
    for (auto tex : thumbnails) {
        SDL_DestroyTexture(tex);
    }
}

void Gallery::scanPhotos() {
    photoPaths.clear();
    for (auto tex : thumbnails) {
        SDL_DestroyTexture(tex);
    }
    thumbnails.clear();

    DIR *d;
    struct dirent *dir;
    d = opendir(PHOTOS_DIR);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            std::string file = dir->d_name;
            if (file.find(".jpg") != std::string::npos || file.find(".avi") != std::string::npos) {
                photoPaths.push_back(std::string(PHOTOS_DIR) + file);
            }
        }
        closedir(d);
    }

    loadThumbnails();
    if (selectedIndex >= (int)photoPaths.size()) selectedIndex = 0;
}

void Gallery::loadThumbnails() {
    for (const auto& path : photoPaths) {
        SDL_Texture* tex = nullptr;
        if (path.find(".avi") != std::string::npos) {
            SDL_Surface* surface = AviPlayer::extractFirstFrame(path);
            if (surface) {
                tex = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
            }
        } else {
            SDL_Surface* surface = IMG_Load(path.c_str());
            if (surface) {
                tex = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
            }
        }
        thumbnails.push_back(tex);
    }
}

void Gallery::render() {
    SDL_SetRenderDrawColor(renderer, 30, 30, 36, 255);
    SDL_RenderClear(renderer);

    SDL_Color titleColor = {0, 210, 110, 255};
    SDL_Color textColor  = {200, 200, 210, 255};

    if (!fullscreen || photoPaths.empty()) {
        if (uiFont) {
            // Title — x=330 so sidebar doesn't cover it
            SDL_Surface* s = TTF_RenderUTF8_Blended(uiFont, I18N::get().galleryTitle.c_str(), titleColor);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
                SDL_Rect r = {330, 28, s->w, s->h};
                SDL_RenderCopy(renderer, t, NULL, &r);
                SDL_DestroyTexture(t); SDL_FreeSurface(s);
            }
        }
    }

    if (photoPaths.empty()) {
        if (uiFont) {
            SDL_Surface* s = TTF_RenderUTF8_Blended(uiFont, I18N::get().galleryEmpty.c_str(), textColor);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
                // Center in the camera area (310 to 1280 -> center is 795)
                SDL_Rect r = {795 - s->w/2, 360 - s->h/2, s->w, s->h};
                SDL_RenderCopy(renderer, t, NULL, &r);
                SDL_DestroyTexture(t); SDL_FreeSurface(s);
            }
        }
        return;
    }

    if (fullscreen) {
        SDL_Rect rect = {310, 0, 970, 720}; 
        bool isVideo = photoPaths[selectedIndex].find(".avi") != std::string::npos;
        
        if (isVideo) {
            aviPlayer.update(renderer);
            SDL_Texture* t = aviPlayer.getTexture();
            if (t) {
                SDL_RenderCopy(renderer, t, NULL, &rect);
                if (!aviPlayer.isPlaying()) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
                    SDL_Rect p1 = {310 + 970/2 - 30, 720/2 - 40, 20, 80};
                    SDL_Rect p2 = {310 + 970/2 + 10, 720/2 - 40, 20, 80};
                    SDL_RenderFillRect(renderer, &p1);
                    SDL_RenderFillRect(renderer, &p2);
                }
            } else {
                if (uiFont) {
                    SDL_Surface* s = TTF_RenderUTF8_Blended(uiFont, "Loading Video...", {255, 255, 255, 255});
                    if (s) {
                        SDL_Texture* tt = SDL_CreateTextureFromSurface(renderer, s);
                        SDL_Rect rr = {310 + 970/2 - s->w/2, 720/2 - s->h/2, s->w, s->h};
                        SDL_RenderCopy(renderer, tt, NULL, &rr);
                        SDL_DestroyTexture(tt);
                        SDL_FreeSurface(s);
                    }
                }
            }
        } else {
            SDL_RenderCopy(renderer, thumbnails[selectedIndex], NULL, &rect);
        }
        
        if (uiFont) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
            SDL_Rect bar = {310, 660, 970, 60};
            SDL_RenderFillRect(renderer, &bar);
            
            std::string fsText = "[Y] " + I18N::get().galleryDelete + "   [A] ";
            fsText += isVideo ? "Play/Pause   [B] Back" : I18N::get().galleryBack;
            SDL_Surface* s = TTF_RenderUTF8_Blended(uiFont, fsText.c_str(), textColor);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
                SDL_Rect r = {795 - s->w/2, 676, s->w, s->h}; // Centered
                SDL_RenderCopy(renderer, t, NULL, &r);
                SDL_DestroyTexture(t);
                SDL_FreeSurface(s);
            }
        }
    } else {
        if (uiFont) {
            // Controls hint at bottom centered
            SDL_Surface* s = TTF_RenderUTF8_Blended(uiFont, I18N::get().galleryControls.c_str(), textColor);
            if (s) {
                SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
                SDL_Rect r = {795 - s->w/2, 680, s->w, s->h};
                SDL_RenderCopy(renderer, t, NULL, &r);
                SDL_DestroyTexture(t); SDL_FreeSurface(s);
            }
        }

        const int startX  = 330;
        const int startY0 = 100;
        const int size    = 200;
        const int padding = 18;
        const int rightEdge = 1280 - 20; // 20px right margin

        int itemsPerRow = (rightEdge - startX) / (size + padding);
        if (itemsPerRow < 1) itemsPerRow = 1; // Guard div-by-zero

        int row = selectedIndex / itemsPerRow;
        // Scroll: shift grid up so selected row is always visible
        int startY = startY0 - (row > 1 ? (row - 1) * (size + padding) : 0);

        int x = startX;
        int y = startY;

        for (size_t i = 0; i < thumbnails.size(); ++i) {
            if (!thumbnails[i]) {
                // We shouldn't hit this if video placeholder works, but just in case
                x += size + padding;
                if (x + size > rightEdge) { x = startX; y += size + padding; }
                continue;
            }
            if (y + size > 80 && y < 720) {
                SDL_Rect r = {x, y, size, size};
                SDL_RenderCopy(renderer, thumbnails[i], NULL, &r);
                
                // Draw Play Icon for videos (Right-facing triangle with black shadow)
                if (photoPaths[i].find(".avi") != std::string::npos) {
                    int cx = x + size/2;
                    int cy = y + size/2;
                    int tSize = 24;

                    // Black shadow/outline
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
                    for (int w = -2; w <= tSize + 1; w++) {
                        int h = tSize - std::max(0, w) + 3;
                        SDL_RenderDrawLine(renderer, cx - 10 + w, cy - h, cx - 10 + w, cy + h);
                    }

                    // White inner triangle
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 240);
                    for (int w = 0; w < tSize; w++) {
                        int h = tSize - w;
                        SDL_RenderDrawLine(renderer, cx - 10 + w, cy - h, cx - 10 + w, cy + h);
                    }
                }

                if ((int)i == selectedIndex) {
                    SDL_SetRenderDrawColor(renderer, 0, 255, 136, 255);
                    SDL_Rect thick1 = {r.x-2, r.y-2, r.w+4, r.h+4};
                    SDL_Rect thick2 = {r.x-3, r.y-3, r.w+6, r.h+6};
                    SDL_Rect thick3 = {r.x-4, r.y-4, r.w+8, r.h+8};
                    SDL_RenderDrawRect(renderer, &thick1);
                    SDL_RenderDrawRect(renderer, &thick2);
                    SDL_RenderDrawRect(renderer, &thick3);
                }
            }

            x += size + padding;
            if (x + size > rightEdge) {
                x = startX;
                y += size + padding;
            }
        }
    }
}

void Gallery::handleInput(u64 kDown) {
    if (photoPaths.empty()) return;

    if (fullscreen) {
        bool isVideo = photoPaths[selectedIndex].find(".avi") != std::string::npos;

        if (kDown & HidNpadButton_B) {
            fullscreen = false;
            if (isVideo) aviPlayer.close();
        }
        if (kDown & HidNpadButton_A) {
            if (isVideo) {
                aviPlayer.togglePlay();
            } else {
                fullscreen = false;
            }
        }
        if (kDown & HidNpadButton_Y) { 
            aviPlayer.close(); // Close BEFORE deleting! Otherwise remove() fails.
            deleteCurrentPhoto(); 
            if (photoPaths.empty()) {
                fullscreen = false;
            } else {
                if (photoPaths[selectedIndex].find(".avi") != std::string::npos) {
                    aviPlayer.load(photoPaths[selectedIndex]);
                }
            }
        }
        
        if (kDown & HidNpadButton_Left) {
            aviPlayer.close();
            selectedIndex--;
            if (selectedIndex < 0) selectedIndex = (int)photoPaths.size() - 1;
            if (photoPaths[selectedIndex].find(".avi") != std::string::npos) aviPlayer.load(photoPaths[selectedIndex]);
        }
        if (kDown & HidNpadButton_Right) {
            aviPlayer.close();
            selectedIndex++;
            if (selectedIndex >= (int)photoPaths.size()) selectedIndex = 0;
            if (photoPaths[selectedIndex].find(".avi") != std::string::npos) aviPlayer.load(photoPaths[selectedIndex]);
        }
    } else {
        if (kDown & HidNpadButton_A) {
            fullscreen = true;
            if (photoPaths[selectedIndex].find(".avi") != std::string::npos) {
                aviPlayer.load(photoPaths[selectedIndex]);
            }
        }
        
        int itemsPerRow = (1280 - 20 - 330) / (200 + 18);
        if (itemsPerRow < 1) itemsPerRow = 1;

        if (kDown & HidNpadButton_Left) {
            if (selectedIndex > 0) selectedIndex--;
        }
        if (kDown & HidNpadButton_Right) {
            if (selectedIndex < (int)photoPaths.size() - 1) selectedIndex++;
        }
        if (kDown & HidNpadButton_Up) {
            if (selectedIndex >= itemsPerRow) selectedIndex -= itemsPerRow;
        }
        if (kDown & HidNpadButton_Down) {
            if (selectedIndex + itemsPerRow < (int)photoPaths.size()) selectedIndex += itemsPerRow;
            else selectedIndex = (int)photoPaths.size() - 1;
        }
        if (kDown & HidNpadButton_Y) {
            deleteCurrentPhoto();
        }
    }
}

void Gallery::deleteCurrentPhoto() {
    if (photoPaths.empty()) return;
    remove(photoPaths[selectedIndex].c_str());
    scanPhotos();
}

bool Gallery::savePhoto(const uint32_t* rgbPixels, int width, int height) {
    mkdir("/switch", 0777);
    mkdir("/switch/CamNX", 0777);
    mkdir(PHOTOS_DIR, 0777);

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char filename[256];
    sprintf(filename, "%sIMG_%04d%02d%02d_%02d%02d%02d.jpg", PHOTOS_DIR, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    FILE* outfile = fopen(filename, "wb");
    if (!outfile) return false;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 90, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1];
    uint8_t* row = new uint8_t[width * 3];
    while (cinfo.next_scanline < cinfo.image_height) {
        int y = cinfo.next_scanline;
        for (int x = 0; x < width; ++x) {
            uint32_t pixel = rgbPixels[y * width + x];
            // ABGR8888: A=bits31-24, B=bits23-16, G=bits15-8, R=bits7-0
            row[x * 3 + 0] = (pixel)       & 0xFF; // R
            row[x * 3 + 1] = (pixel >> 8)  & 0xFF; // G
            row[x * 3 + 2] = (pixel >> 16) & 0xFF; // B
        }
        row_pointer[0] = row;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    delete[] row;
    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);

    return true;
}
