#include "filters.hpp"
#include <stdlib.h>

// Pixel format: SDL_PIXELFORMAT_ABGR8888
// uint32_t layout (MSB to LSB): A | B | G | R
// So pixel = (0xFF << 24) | (B << 16) | (G << 8) | R
// This was the confirmed-working format on Switch.

void FilterEngine::applyFilter(FilterMode mode, const uint8_t* irData, uint32_t* rgbPixels, int width, int height) {
    int length = width * height;
    switch (mode) {
        case FilterMode::RAW_BW:       applyRawBW(irData, rgbPixels, length);              break;
        case FilterMode::NIGHT_VISION: applyNightVision(irData, rgbPixels, length);        break;
        case FilterMode::THERMAL:      applyThermal(irData, rgbPixels, length);            break;
        case FilterMode::MATRIX:       applyMatrix(irData, rgbPixels, width, height);      break;
        case FilterMode::BARBIE:       applyBarbie(irData, rgbPixels, length);             break;
        case FilterMode::SEPIA:        applySepia(irData, rgbPixels, length);              break;
        case FilterMode::CYBERPUNK:    applyCyberpunk(irData, rgbPixels, length);          break;
        default:                       applyRawBW(irData, rgbPixels, length);              break;
    }
}

void FilterEngine::applyRawBW(const uint8_t* irData, uint32_t* rgbPixels, int length) {
    for (int i = 0; i < length; ++i) {
        uint8_t v = irData[i];
        // ABGR8888: A=0xFF, B=v, G=v, R=v
        rgbPixels[i] = 0xFF000000u | ((uint32_t)v << 16) | ((uint32_t)v << 8) | v;
    }
}

void FilterEngine::applyNightVision(const uint8_t* irData, uint32_t* rgbPixels, int length) {
    for (int i = 0; i < length; ++i) {
        uint8_t v = irData[i];
        uint8_t r = v >> 2;                               // dim red
        uint8_t g = v < 205 ? (uint8_t)(v + 50) : 255;  // boosted green
        uint8_t b = v >> 3;                               // minimal blue
        // ABGR8888: A | B | G | R
        rgbPixels[i] = 0xFF000000u | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
    }
}

void FilterEngine::applyThermal(const uint8_t* irData, uint32_t* rgbPixels, int length) {
    for (int i = 0; i < length; ++i) {
        uint8_t val = irData[i];
        uint8_t r, g, b;
        if (val < 64) {
            r = 0;  g = 0;  b = (uint8_t)(val * 4);
        } else if (val < 128) {
            r = 0;  g = (uint8_t)((val - 64) * 4);  b = 255;
        } else if (val < 192) {
            r = (uint8_t)((val - 128) * 4);  g = 255;  b = (uint8_t)(255 - (val - 128) * 4);
        } else {
            r = 255;  g = (uint8_t)(255 - (val - 192) * 4);  b = 0;
        }
        rgbPixels[i] = 0xFF000000u | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
    }
}

void FilterEngine::applyMatrix(const uint8_t* irData, uint32_t* rgbPixels, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t val = irData[y * width + x];
            int bright = (int)(val * 1.6f);
            if (bright > 255) bright = 255;
            if (y % 3 == 0) bright /= 3;  // scanline effect
            uint8_t g = (uint8_t)bright;
            // ABGR8888: A=0xFF, B=0, G=g, R=0  → pure green
            rgbPixels[y * width + x] = 0xFF000000u | ((uint32_t)g << 8);
        }
    }
}

void FilterEngine::applyBarbie(const uint8_t* irData, uint32_t* rgbPixels, int length) {
    for (int i = 0; i < length; ++i) {
        uint8_t v = irData[i];
        // Boost red and blue, lower green
        uint8_t r = (uint8_t)(v * 1.3f); if (r < v) r = 255;
        uint8_t g = (uint8_t)(v * 0.4f);
        uint8_t b = (uint8_t)(v * 1.1f); if (b < v) b = 255;
        rgbPixels[i] = 0xFF000000u | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
    }
}

void FilterEngine::applySepia(const uint8_t* irData, uint32_t* rgbPixels, int length) {
    for (int i = 0; i < length; ++i) {
        uint8_t v = irData[i];
        // Classic sepia tone
        uint8_t r = (uint8_t)(v * 1.1f); if (r < v) r = 255;
        uint8_t g = (uint8_t)(v * 0.9f);
        uint8_t b = (uint8_t)(v * 0.6f);
        rgbPixels[i] = 0xFF000000u | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
    }
}

void FilterEngine::applyCyberpunk(const uint8_t* irData, uint32_t* rgbPixels, int length) {
    for (int i = 0; i < length; ++i) {
        uint8_t v = irData[i];
        uint8_t r, g, b;
        if (v < 128) {
            // Dark regions: deep blue / purple
            r = (uint8_t)(v * 0.5f);
            g = 0;
            b = (uint8_t)(v * 1.5f);
        } else {
            // Bright regions: neon pink / cyan tint
            r = 255;
            g = (uint8_t)((v - 128) * 1.5f);
            b = (uint8_t)(255 - ((v - 128) * 1.5f));
        }
        rgbPixels[i] = 0xFF000000u | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
    }
}
