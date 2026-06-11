#include "avi_writer.hpp"
#include <jpeglib.h>
#include <stdlib.h>
#include <string.h>

void AviWriter::writeU32(uint32_t v) { fwrite(&v, 1, 4, file); }
void AviWriter::writeU16(uint16_t v) { fwrite(&v, 1, 2, file); }
void AviWriter::writeFourCC(const char* fcc) { fwrite(fcc, 1, 4, file); }

AviWriter::AviWriter(const std::string& filename, int w, int h, int f) 
    : width(w), height(h), fps(f), framesWritten(0), moviListOffset(0) {
    file = fopen(filename.c_str(), "wb");
    if (file) {
        writeHeader();
    }
}

AviWriter::~AviWriter() {
    if (file) {
        finish();
    }
}

void AviWriter::writeHeader() {
    fseek(file, 0, SEEK_SET);
    writeFourCC("RIFF");
    writeU32(0); // File size - 8 (placeholder)
    writeFourCC("AVI ");

    writeFourCC("LIST");
    writeU32(192); // hdrl size
    writeFourCC("hdrl");

    writeFourCC("avih");
    writeU32(56);
    writeU32(1000000 / fps); // Microseconds per frame
    writeU32(0); // Max bytes per sec
    writeU32(0); // Padding granularity
    writeU32(0x10); // Flags (has index)
    writeU32(framesWritten); // Total frames
    writeU32(0); // Initial frames
    writeU32(1); // Streams
    writeU32(0); // Suggested buffer size
    writeU32(width);
    writeU32(height);
    writeU32(0); // Reserved
    writeU32(0); // Reserved
    writeU32(0); // Reserved
    writeU32(0); // Reserved

    writeFourCC("LIST");
    writeU32(116); // strl size
    writeFourCC("strl");

    writeFourCC("strh");
    writeU32(56);
    writeFourCC("vids");
    writeFourCC("MJPG");
    writeU32(0); // Flags
    writeU16(0); // Priority
    writeU16(0); // Language
    writeU32(0); // Initial frames
    writeU32(1); // Scale
    writeU32(fps); // Rate
    writeU32(0); // Start
    writeU32(framesWritten); // Length
    writeU32(0); // Suggested buffer size
    writeU32(10000); // Quality
    writeU32(0); // Sample size
    writeU16(0); // Frame coords
    writeU16(0); 
    writeU16(width);
    writeU16(height);

    writeFourCC("strf");
    writeU32(40);
    writeU32(40); // Size
    writeU32(width);
    writeU32(height);
    writeU16(1); // Planes
    writeU16(24); // Bitcount
    writeFourCC("MJPG"); // Compression
    writeU32(width * height * 3); // SizeImage
    writeU32(0); // XPelsPerMeter
    writeU32(0); // YPelsPerMeter
    writeU32(0); // ClrUsed
    writeU32(0); // ClrImportant

    writeFourCC("LIST");
    moviListOffset = ftell(file);
    writeU32(0); // movi size placeholder
    writeFourCC("movi");
}

bool AviWriter::addFrame(const uint32_t* rgbPixels) {
    if (!file) return false;

    // Encode to JPEG in memory
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    unsigned char* mem = nullptr;
    unsigned long memSize = 0;
    jpeg_mem_dest(&cinfo, &mem, &memSize);

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
            row[x * 3 + 0] = (pixel)       & 0xFF; // R
            row[x * 3 + 1] = (pixel >> 8)  & 0xFF; // G
            row[x * 3 + 2] = (pixel >> 16) & 0xFF; // B
        }
        row_pointer[0] = row;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    delete[] row;
    
    // Ensure 2-byte alignment for chunk sizes
    uint32_t chunkSize = memSize;
    uint32_t paddedSize = (chunkSize + 1) & ~1;

    long chunkOffset = ftell(file);
    writeFourCC("00dc");
    writeU32(chunkSize);
    fwrite(mem, 1, chunkSize, file);
    if (paddedSize > chunkSize) {
        uint8_t pad = 0;
        fwrite(&pad, 1, 1, file);
    }

    IndexEntry ie;
    ie.chunkId = 0x63643030; // '00dc'
    ie.flags = 0x10; // Keyframe
    ie.offset = chunkOffset - (moviListOffset - 4); // Offset from 'LIST' start
    ie.size = chunkSize;
    index.push_back(ie);

    framesWritten++;

    free(mem);
    jpeg_destroy_compress(&cinfo);

    return true;
}

void AviWriter::finish() {
    if (!file) return;

    long endOfMovi = ftell(file);
    
    // Write idx1
    writeFourCC("idx1");
    writeU32(index.size() * 16);
    for (const auto& ie : index) {
        writeU32(ie.chunkId);
        writeU32(ie.flags);
        writeU32(ie.offset);
        writeU32(ie.size);
    }

    long endOfFile = ftell(file);

    // Update headers
    fseek(file, 4, SEEK_SET);
    writeU32(endOfFile - 8);

    fseek(file, 48, SEEK_SET);
    writeU32(framesWritten);

    fseek(file, 140, SEEK_SET);
    writeU32(framesWritten);

    fseek(file, moviListOffset, SEEK_SET);
    writeU32(endOfMovi - moviListOffset - 4);

    fclose(file);
    file = nullptr;
}
