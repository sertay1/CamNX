#pragma once
#include <switch.h>
#include <stdint.h>
#include <stdbool.h>

#define IR_WIDTH 320
#define IR_HEIGHT 240
#define IR_BUFFER_SIZE (IR_WIDTH * IR_HEIGHT)

class IRCamera {
public:
    IRCamera();
    ~IRCamera();

    bool initialize(PadState* pad);
    void finalize();
    bool updateFrame(PadState* pad);
    uint8_t* getFrameBuffer();
    void setResolution(int resMode);
    int getResolution() const;

private:
    IrsIrCameraHandle handle;
    uint8_t* workBuffer;
    uint8_t* frameBuffer;
    bool isInitialized;
    bool irsStarted;      // true if irsInitialize() succeeded
    int reconnectTimer;
    bool lastIsHandheld;
    int currentResolution; // 0=320x240, 1=160x120, 2=80x60, 3=40x30

    bool reconnect();
};
