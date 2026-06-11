#pragma once
#include <SDL2/SDL.h>
#include <switch.h>
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "avi_player.hpp"

class Gallery {
public:
    Gallery(SDL_Renderer* ren, TTF_Font* font);
    ~Gallery();

    void scanPhotos();
    void updateFonts(TTF_Font* newFont) { uiFont = newFont; }
    void render();
    void handleInput(u64 kDown);
    int getPhotoCount() const { return photoPaths.size(); }
    static bool savePhoto(const uint32_t* rgbPixels, int width, int height);

private:
    SDL_Renderer* renderer;
    TTF_Font* uiFont;
    std::vector<std::string> photoPaths;
    std::vector<SDL_Texture*> thumbnails;
    int selectedIndex;
    bool fullscreen;
    AviPlayer aviPlayer;

    void loadThumbnails();
    void deleteCurrentPhoto();
};
