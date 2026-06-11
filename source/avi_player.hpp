#pragma once

#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

class AviPlayer {
public:
    AviPlayer();
    ~AviPlayer();

    // Loads an AVI file and prepares it for playback.
    bool load(const std::string& path);
    
    // Static helper to get the first frame as an SDL_Surface (for thumbnails)
    static SDL_Surface* extractFirstFrame(const std::string& path);
    
    // Closes the currently loaded file and frees resources.
    void close();
    
    // Call this every frame to update the current texture.
    void update(SDL_Renderer* renderer);
    
    // Returns the current frame texture.
    SDL_Texture* getTexture() const { return currentTexture; }
    
    // Player controls
    bool isPlaying() const { return playing; }
    void togglePlay() { playing = !playing; }
    void rewind();

private:
    FILE* file = nullptr;
    bool playing = false;
    uint32_t lastFrameTime = 0;
    
    long moviStart = 0;
    long moviEnd = 0;

    SDL_Texture* currentTexture = nullptr;
    
    bool readNextFrame(SDL_Renderer* renderer);
};
