#pragma once

#include <string>
#include <stdio.h>
#include <stdint.h>
#include <vector>

class AviWriter {
public:
    AviWriter(const std::string& filename, int width, int height, int fps);
    ~AviWriter();

    bool addFrame(const uint32_t* rgbPixels);
    void finish();

    bool isOk() const { return file != nullptr; }
    
    int getDurationSeconds() const {
        if (fps == 0) return 0;
        return framesWritten / fps;
    }

private:
    FILE* file;
    int width;
    int height;
    int fps;
    int framesWritten;
    
    long moviListOffset;
    
    struct IndexEntry {
        uint32_t chunkId;
        uint32_t flags;
        uint32_t offset;
        uint32_t size;
    };
    std::vector<IndexEntry> index;

    void writeU32(uint32_t v);
    void writeU16(uint16_t v);
    void writeFourCC(const char* fcc);
    
    void writeHeader();
    void updateHeader();
};
