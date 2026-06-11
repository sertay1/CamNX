#include "camera.hpp"
#include <malloc.h>
#include <string.h>
#include <stdio.h>

IRCamera::IRCamera() : workBuffer(nullptr), frameBuffer(nullptr), isInitialized(false), irsStarted(false), reconnectTimer(0), lastIsHandheld(false), currentResolution(0) {}

IRCamera::~IRCamera() {
    finalize();
}

bool IRCamera::initialize(PadState* pad) {
    if (R_FAILED(irsInitialize())) {
        return false;
    }
    irsStarted = true;

    lastIsHandheld = padIsHandheld(pad);
    HidNpadIdType id = lastIsHandheld ? HidNpadIdType_Handheld : HidNpadIdType_No1;
    
    if (R_FAILED(irsGetIrCameraHandle(&handle, id))) {
        irsExit();
        irsStarted = false;
        return false;
    }

    size_t workBufSize = 0x100000;
    workBuffer = (uint8_t*)memalign(0x1000, workBufSize);
    frameBuffer = new uint8_t[IR_BUFFER_SIZE];

    if (!workBuffer || !frameBuffer) {
        if (workBuffer) { free(workBuffer); workBuffer = nullptr; }
        if (frameBuffer) { delete[] frameBuffer; frameBuffer = nullptr; }
        irsExit();
        irsStarted = false;
        return false;
    }
    memset(frameBuffer, 128, IR_BUFFER_SIZE); // Default grey

    IrsImageTransferProcessorConfig config;
    irsGetDefaultImageTransferProcessorConfig(&config);
    if (currentResolution == 3) config.format = IrsImageTransferProcessorFormat_320x240;
    else if (currentResolution == 2) config.format = IrsImageTransferProcessorFormat_160x120;
    else if (currentResolution == 1) config.format = IrsImageTransferProcessorFormat_80x60;
    else config.format = IrsImageTransferProcessorFormat_40x30;

    if (R_FAILED(irsRunImageTransferProcessor(handle, &config, workBufSize))) {
        irsExit();
        irsStarted = false;
        return false;
    }

    isInitialized = true;
    return true;
}

bool IRCamera::reconnect() {
    // Left empty or re-init logic since initialize() does everything
    return true;
}

void IRCamera::finalize() {
    if (isInitialized) {
        irsStopImageProcessor(handle);
        isInitialized = false;
    }
    if (irsStarted) {
        irsExit();
        irsStarted = false;
    }
    if (workBuffer)  { free(workBuffer);     workBuffer  = nullptr; }
    if (frameBuffer) { delete[] frameBuffer; frameBuffer = nullptr; }
}

bool IRCamera::updateFrame(PadState* pad) {
    if (!workBuffer || !frameBuffer || !isInitialized) return false;

    bool currentIsHandheld = padIsHandheld(pad);
    if (currentIsHandheld != lastIsHandheld) {
        // Connection type changed (attached vs wireless)
        finalize();
        if (initialize(pad)) {
            return false; // Skip this frame while reconnecting
        }
        return false;
    }

    IrsImageTransferProcessorState state;
    static uint64_t last_sampling_number = UINT64_MAX;

    size_t expectedSize;
    int srcW, srcH, scale;
    if (currentResolution == 3) { expectedSize = 320 * 240; srcW = 320; srcH = 240; scale = 1; }
    else if (currentResolution == 2) { expectedSize = 160 * 120; srcW = 160; srcH = 120; scale = 2; }
    else if (currentResolution == 1) { expectedSize = 80 * 60; srcW = 80; srcH = 60; scale = 4; }
    else { expectedSize = 40 * 30; srcW = 40; srcH = 30; scale = 8; }

    Result rc = irsGetImageTransferProcessorState(handle, workBuffer, expectedSize, &state);

    if (R_FAILED(rc)) {
        return false;
    }

    if (state.sampling_number != last_sampling_number) {
        last_sampling_number = state.sampling_number;

        if (scale == 1) {
            // Native 320x240: just copy directly, no CPU upscaling needed!
            memcpy(frameBuffer, workBuffer, IR_WIDTH * IR_HEIGHT);
        } else {
            // CPU Upscaling for lower resolutions
            for (int y = 0; y < srcH; y++) {
                for (int x = 0; x < srcW; x++) {
                    uint8_t pixel = workBuffer[y * srcW + x];
                    int dstY = y * scale;
                    int dstX = x * scale;
                    for (int dy = 0; dy < scale; dy++) {
                        for (int dx = 0; dx < scale; dx++) {
                            frameBuffer[(dstY + dy) * IR_WIDTH + (dstX + dx)] = pixel;
                        }
                    }
                }
            }
        }
        
        return true;
    }
    return false;
}

void IRCamera::setResolution(int resMode) {
    if (currentResolution == resMode) return;
    currentResolution = resMode;
    // We can gracefully reboot the processor
    if (isInitialized) {
        irsStopImageProcessor(handle);
        IrsImageTransferProcessorConfig config;
        irsGetDefaultImageTransferProcessorConfig(&config);
        if (currentResolution == 3) config.format = IrsImageTransferProcessorFormat_320x240;
        else if (currentResolution == 2) config.format = IrsImageTransferProcessorFormat_160x120;
        else if (currentResolution == 1) config.format = IrsImageTransferProcessorFormat_80x60;
        else config.format = IrsImageTransferProcessorFormat_40x30;
        
        size_t workBufSize = 0x100000;
        irsRunImageTransferProcessor(handle, &config, workBufSize);
    }
}

int IRCamera::getResolution() const {
    return currentResolution;
}

uint8_t* IRCamera::getFrameBuffer() {
    return frameBuffer;
}
