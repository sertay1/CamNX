#pragma once
#include <stdint.h>
#include <SDL2/SDL.h>

enum class FilterMode {
    RAW_BW,
    NIGHT_VISION,
    THERMAL,
    MATRIX,
    BARBIE,
    SEPIA,
    CYBERPUNK,
    NUM_FILTERS
};

class FilterEngine {
public:
    static void applyFilter(FilterMode mode, const uint8_t* irData, uint32_t* rgbPixels, int width, int height);

private:
    static void applyRawBW(const uint8_t* irData, uint32_t* rgbPixels, int length);
    static void applyNightVision(const uint8_t* irData, uint32_t* rgbPixels, int length);
    static void applyThermal(const uint8_t* irData, uint32_t* rgbPixels, int length);
    static void applyMatrix(const uint8_t* irData, uint32_t* rgbPixels, int width, int height);
    static void applyBarbie(const uint8_t* irData, uint32_t* rgbPixels, int length);
    static void applySepia(const uint8_t* irData, uint32_t* rgbPixels, int length);
    static void applyCyberpunk(const uint8_t* irData, uint32_t* rgbPixels, int length);
};
