#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>

#include "camera.hpp"
#include "filters.hpp"
#include "gallery.hpp"
#include "server.hpp"
#include "i18n.hpp"
#include "avi_writer.hpp"
#include <time.h>

enum class AppState { CAMERA, GALLERY, SHARE, LANGUAGE };

// Global Fonts
TTF_Font* fontLg = nullptr;
TTF_Font* fontSm = nullptr;

// ── Debug log ────────────────────────────────────────────────────────────────
void dbg_log(const char* msg) {
    FILE* f = fopen("/switch/CamNX_log.txt", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

void reloadFonts(Language lang) {
    if (fontLg) { TTF_CloseFont(fontLg); fontLg = nullptr; }
    if (fontSm) { TTF_CloseFont(fontSm); fontSm = nullptr; }

    PlSharedFontType type = PlSharedFontType_Standard;
    if (lang == Language::KOREAN) type = PlSharedFontType_KO;

    PlFontData fontData;
    if (R_SUCCEEDED(plGetSharedFontByType(&fontData, type))) {
        SDL_RWops* rw1 = SDL_RWFromMem(fontData.address, fontData.size);
        if (rw1) fontLg = TTF_OpenFontRW(rw1, 1, 26);
        SDL_RWops* rw2 = SDL_RWFromMem(fontData.address, fontData.size);
        if (rw2) fontSm = TTF_OpenFontRW(rw2, 1, 20);
        dbg_log("Fonts reloaded successfully.");
    } else {
        dbg_log("Failed to reload fonts.");
    }
}

// ── SDL helpers ──────────────────────────────────────────────────────────────
static void renderText(SDL_Renderer* r, TTF_Font* f, const char* txt, int x, int y, SDL_Color c) {
    if (!f || !txt || txt[0] == '\0') return;
    SDL_Surface* s = TTF_RenderUTF8_Blended(f, txt, c);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
    if (t) {
        SDL_Rect d = {x, y, s->w, s->h};
        SDL_RenderCopy(r, t, NULL, &d);
        SDL_DestroyTexture(t);
    }
    SDL_FreeSurface(s);
}

static void renderTextCentered(SDL_Renderer* r, TTF_Font* f, const char* txt, int cx, int y, SDL_Color c) {
    if (!f || !txt || txt[0] == '\0') return;
    SDL_Surface* s = TTF_RenderUTF8_Blended(f, txt, c);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
    if (t) {
        SDL_Rect d = {cx - s->w / 2, y, s->w, s->h};
        SDL_RenderCopy(r, t, NULL, &d);
        SDL_DestroyTexture(t);
    }
    SDL_FreeSurface(s);
}

static void drawHLine(SDL_Renderer* r, int x, int y, int w, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderDrawLine(r, x, y, x + w - 1, y);
    SDL_SetRenderDrawColor(r, c.r / 4, c.g / 4, c.b / 4, c.a);
    SDL_RenderDrawLine(r, x, y + 1, x + w - 1, y + 1);
}

// ── Sidebar ──────────────────────────────────────────────────────────────────
#define PANEL_W 310

static void drawSidebar(SDL_Renderer* renderer, AppState currentState, int menuIndex, FilterMode currentFilter, int currentResolution, SDL_Texture* logo) {
    const auto& i18n = I18N::get();
    
    SDL_SetRenderDrawColor(renderer, 24, 24, 32, 255);
    SDL_Rect sidebar = {0, 0, PANEL_W, 720};
    SDL_RenderFillRect(renderer, &sidebar);

    const SDL_Color cAccent  = {0,  210, 110, 255};
    const SDL_Color cWhite   = {230,230, 240, 255};
    const SDL_Color cGrey    = {130,130, 155, 255};
    const SDL_Color cDim     = { 38, 38,  50, 255};

    int y = 30;

    // ---------- Logo / App name ----------
    if (logo) {
        int lw, lh;
        SDL_QueryTexture(logo, NULL, NULL, &lw, &lh);
        int dw = 240; 
        int dh = (dw * lh) / lw;
        if (dh > 130) { dh = 130; dw = (dh * lw) / lh; }
        SDL_Rect lr = {(PANEL_W - dw) / 2, y, dw, dh};
        SDL_RenderCopy(renderer, logo, NULL, &lr);
        y += dh + 5;
    } else {
        renderTextCentered(renderer, fontLg, i18n.appName.c_str(), PANEL_W / 2, y, cAccent);
        y += 50;
    }

    drawHLine(renderer, 14, y, PANEL_W - 28, cDim);
    y += 10;

    // ---------- FILTER section ----------
    renderText(renderer, fontSm, i18n.filterMode.c_str(), 16, y, cGrey);
    y += 24;

    // Active filter name pill
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 195, 100, 28);
    SDL_Rect pill = {12, y - 2, PANEL_W - 24, 34};
    SDL_RenderFillRect(renderer, &pill);
    SDL_SetRenderDrawColor(renderer, 0, 195, 100, 160);
    SDL_RenderDrawRect(renderer, &pill);

    const char* filterName = i18n.filterBW.c_str();
    switch (currentFilter) {
        case FilterMode::NIGHT_VISION: filterName = i18n.filterNight.c_str();     break;
        case FilterMode::THERMAL:      filterName = i18n.filterThermal.c_str();   break;
        case FilterMode::MATRIX:       filterName = i18n.filterMatrix.c_str();    break;
        case FilterMode::BARBIE:       filterName = i18n.filterBarbie.c_str();    break;
        case FilterMode::SEPIA:        filterName = i18n.filterSepia.c_str();     break;
        case FilterMode::CYBERPUNK:    filterName = i18n.filterCyberpunk.c_str(); break;
        default: break;
    }
    renderText(renderer, fontLg, filterName, 20, y, cAccent);
    y += 40;

    // Filter progress dots
    {
        const int nF   = (int)FilterMode::NUM_FILTERS;
        const int cur  = (int)currentFilter;
        int dotX = 18;
        for (int i = 0; i < nF; i++) {
            bool active = (i == cur);
            SDL_SetRenderDrawColor(renderer,
                active ? 0   : 55,
                active ? 200 : 60,
                active ? 100 : 70,
                255);
            SDL_Rect dot = {dotX, y + 5, active ? 20 : 8, 8};
            SDL_RenderFillRect(renderer, &dot);
            dotX += (active ? 20 : 8) + 8;
        }
    }
    y += 24;

    drawHLine(renderer, 14, y, PANEL_W - 28, cDim);
    y += 10;

    // ---------- RESOLUTION section ----------
    renderText(renderer, fontSm, i18n.ctrlResolution.c_str(), 14, y, cGrey);
    y += 24;

    const char* resOptions[4] = { "40x30", "80x60", "160x120", "320x240" };
    int curRes = currentResolution;
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 195, 100, 30);
    SDL_Rect resBox = { 14, y - 2, PANEL_W - 28, 28 };
    SDL_RenderFillRect(renderer, &resBox);
    SDL_SetRenderDrawColor(renderer, 0, 195, 100, 255);
    SDL_RenderDrawRect(renderer, &resBox);

    renderText(renderer, fontLg, resOptions[curRes], 22, y + 2, cWhite);
    y += 32;

    // Resolution dots (Small to Large)
    {
        int dotX = 14;
        for (int i = 0; i <= 3; i++) {
            bool active = (i == curRes);
            SDL_SetRenderDrawColor(renderer,
                active ? 0 : 100,
                active ? 195 : 100,
                active ? 100 : 100,
                255);
            SDL_Rect dot = {dotX, y + 5, active ? 20 : 8, 8};
            SDL_RenderFillRect(renderer, &dot);
            dotX += (active ? 20 : 8) + 8;
        }
    }
    y += 20;

    SDL_Color lineCol = {120, 120, 130, 255};
    drawHLine(renderer, 14, y, PANEL_W - 28, lineCol);
    y += 10;

    // ---------- NAVIGATION section ----------
    y += 4;

    struct NavItem { const char* label; AppState target; };
    const NavItem navItems[4] = {
        {i18n.navCamera.c_str(),  AppState::CAMERA},
        {i18n.navGallery.c_str(), AppState::GALLERY},
        {i18n.navShare.c_str(),   AppState::SHARE},
        {i18n.navLanguage.c_str(),AppState::LANGUAGE},
    };

    for (int i = 0; i < 4; i++) {
        bool active = (menuIndex == i);

        if (active) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 195, 100, 20);
            SDL_Rect rowBg = {12, y - 2, PANEL_W - 24, 32};
            SDL_RenderFillRect(renderer, &rowBg);
            SDL_SetRenderDrawColor(renderer, 0, 195, 100, 255);
            SDL_Rect indicator = {12, y - 2, 3, 32};
            SDL_RenderFillRect(renderer, &indicator);
        }

        renderText(renderer, fontSm, navItems[i].label, 24, y + 4, active ? cWhite : cGrey);
        y += 36;
    }

    drawHLine(renderer, 14, y, PANEL_W - 28, lineCol);
    y += 20;

    // ---------- Controls (bottom region) ----------
    renderText(renderer, fontSm, i18n.ctrlTitle.c_str(), 16, y, cGrey); // "Kontroller"
    y += 22;
    renderText(renderer, fontSm, i18n.ctrlPhoto.c_str(),   16, y, cGrey); y += 22; // A Take Photo / Enter
    renderText(renderer, fontSm, "[B] Video Record",       16, y, cGrey); y += 22; // B Record Video
    renderText(renderer, fontSm, i18n.ctrlFilter.c_str(),  16, y, cGrey); y += 22; // L/R Filters
    renderText(renderer, fontSm, i18n.ctrlZL_ZR.c_str(),   16, y, cGrey); y += 22; // ZL/ZR Switch Menu
    renderText(renderer, fontSm, (std::string("[X] ") + i18n.ctrlResolution).c_str(), 16, y, cGrey); y += 22; // X Resolution
    renderText(renderer, fontSm, i18n.ctrlExit.c_str(),    16, y, cGrey); y += 22; // - Exit
    
    // ---------- Footer ----------
    renderTextCentered(renderer, fontSm, "by SertAy", PANEL_W / 2, 690, cAccent); // Made by SertAy prominent
}

// ── Share screen ─────────────────────────────────────────────────────────────
static void drawShareScreen(SDL_Renderer* r, Server& server) {
    SDL_SetRenderDrawColor(r, 14, 14, 20, 255);
    SDL_Rect bg = {PANEL_W, 0, 1280 - PANEL_W, 720};
    SDL_RenderFillRect(r, &bg);

    const SDL_Color cAccent  = {0, 210, 110, 255};
    const SDL_Color cWhite   = {230,230,240, 255};

    int cx = PANEL_W + (1280 - PANEL_W) / 2;

    renderTextCentered(r, fontLg, I18N::get().shareTitle.c_str(), cx, 38, cAccent);

    SDL_SetRenderDrawColor(r, 38, 38, 50, 255);
    SDL_RenderDrawLine(r, PANEL_W + 50, 76, 1230, 76);

    int leftCx = PANEL_W + 970 / 4 + 30; 
    int rightCx = PANEL_W + 970 * 3 / 4 - 30;
    int cy = 340;

    renderTextCentered(r, fontSm, I18N::get().shareDesc.c_str(), leftCx, cy - 60, cWhite);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 195, 100, 20);
    SDL_Rect ipBox = {leftCx - 200, cy - 20, 400, 50};
    SDL_RenderFillRect(r, &ipBox);
    SDL_SetRenderDrawColor(r, 0, 195, 100, 140);
    SDL_RenderDrawRect(r, &ipBox);

    std::string ip = server.getIP();
    if (ip.empty() || ip == "0.0.0.0") ip = "Not connected";
    else ip += ":8080";
    renderTextCentered(r, fontLg, ip.c_str(), leftCx, cy - 6, cWhite);

    server.renderQR(rightCx, cy);
}


// ── Language screen ──────────────────────────────────────────────────────────
static void drawLanguageScreen(SDL_Renderer* r, int selectedIndex) {
    SDL_SetRenderDrawColor(r, 14, 14, 20, 255);
    SDL_Rect bg = {PANEL_W, 0, 1280 - PANEL_W, 720};
    SDL_RenderFillRect(r, &bg);

    const SDL_Color cAccent  = {0, 210, 110, 255};
    const SDL_Color cWhite   = {230,230,240, 255};
    const SDL_Color cGrey    = {130,130,155, 255};

    int cx = PANEL_W + (1280 - PANEL_W) / 2;

    renderTextCentered(r, fontLg, I18N::get().selectLang.c_str(), cx, 38, cAccent);

    SDL_SetRenderDrawColor(r, 38, 38, 50, 255);
    SDL_RenderDrawLine(r, PANEL_W + 50, 76, 1230, 76);

    int startY = 120;
    int itemH = 45;
    for (int i = 0; i < (int)Language::NUM_LANGUAGES; i++) {
        bool active = (i == selectedIndex);
        if (active) {
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(r, 0, 195, 100, 30);
            SDL_Rect rowBg = {cx - 200, startY + i * itemH - 8, 400, itemH + 6};
            SDL_RenderFillRect(r, &rowBg);
            SDL_SetRenderDrawColor(r, 0, 195, 100, 255);
            SDL_Rect indicator = {cx - 200, startY + i * itemH - 8, 4, itemH + 6};
            SDL_RenderFillRect(r, &indicator);
        }

        std::string name = I18N::getNativeLanguageName((Language)i);
        renderTextCentered(r, active ? fontLg : fontSm, name.c_str(), cx, startY + i * itemH + (active ? 0 : 4), active ? cWhite : cGrey);
    }

    SDL_SetRenderDrawColor(r, 38, 38, 50, 255);
    SDL_RenderDrawLine(r, PANEL_W + 50, 640, 1230, 640);
    renderTextCentered(r, fontSm, I18N::get().langHint.c_str(), cx, 660, cGrey);
}

// ── Main ─────────────────────────────────────────────────────────────────────
int main(int argc, char **argv) {
    remove("/switch/CamNX_log.txt");
    dbg_log("App Started.");
    I18N::init();

    socketInitializeDefault(); dbg_log("socket done.");
    nifmInitialize(NifmServiceType_User); dbg_log("nifm done.");
    plInitialize(PlServiceType_User);     dbg_log("pl done.");
    romfsInit();                          dbg_log("romfs done.");
    appletSetMediaPlaybackState(true);    dbg_log("media playback state true.");

    // INPUT INIT BEFORE CAMERA! This fixes the camera handle bug.
    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);
    dbg_log("Pad initialized.");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        dbg_log("SDL_Init failed."); return -1;
    }
    dbg_log("SDL_Init done.");

    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    TTF_Init();
    dbg_log("IMG/TTF Init done.");

    SDL_Window* window = SDL_CreateWindow("CamNX",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN);
    if (!window) { dbg_log("SDL_CreateWindow failed."); return -1; }
    dbg_log("SDL_CreateWindow done.");

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { dbg_log("SDL_CreateRenderer failed."); return -1; }
    dbg_log("SDL_CreateRenderer done.");

    reloadFonts(I18N::getLanguage());

    // ── Camera
    dbg_log("Initializing Camera...");
    IRCamera camera;
    camera.initialize(&pad);
    dbg_log("Camera done.");

    // ── Gallery
    dbg_log("Initializing Gallery...");
    Gallery gallery(renderer, fontLg);
    dbg_log("Gallery done.");

    // ── Server
    dbg_log("Initializing Server...");
    Server server(renderer);
    dbg_log("Server done.");

    AppState currentState = AppState::CAMERA;
    int menuIndex = 0;
    bool running = true;
    FilterMode currentFilter = FilterMode::RAW_BW;
    int langSelection = (int)I18N::getLanguage();

    // Camera texture
    SDL_Texture* camTexture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
        IR_WIDTH, IR_HEIGHT);
    if (!camTexture) dbg_log("camTexture creation FAILED!");

    uint32_t* rgbPixels = new uint32_t[IR_WIDTH * IR_HEIGHT];
    for (int i = 0; i < IR_WIDTH * IR_HEIGHT; i++)
        rgbPixels[i] = 0xFF808080u; // ABGR: opaque grey
    SDL_UpdateTexture(camTexture, NULL, rgbPixels, IR_WIDTH * sizeof(uint32_t));

    SDL_Texture* appLogo = IMG_LoadTexture(renderer, "romfs:/camnx_uygulama_ici.png");
    dbg_log(appLogo ? "Logo loaded OK." : "Logo NOT found.");

    dbg_log("Entering main loop...");
    bool firstFrame = true;

    bool isRecording = false;
    AviWriter* videoWriter = nullptr;
    uint32_t recordStartTime = 0;
    uint32_t nextVideoFrameTime = 0;
    const uint32_t TARGET_FPS = 15;
    const uint32_t FRAME_DELAY_MS = 1000 / TARGET_FPS;


    while (appletMainLoop() && running) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        // Map Minus to Exit
        if (kDown & HidNpadButton_Minus) running = false;
        
        bool lrChanged = false;
        if (kDown & HidNpadButton_ZL) { menuIndex--; if (menuIndex < 0) menuIndex = 3; lrChanged = true; }
        if (kDown & HidNpadButton_ZR) { menuIndex++; if (menuIndex > 3) menuIndex = 0; lrChanged = true; }

        if (lrChanged) {
            if (menuIndex == 0) {
                currentState = AppState::CAMERA;
                server.stop();
            } else if (menuIndex == 1) {
                currentState = AppState::GALLERY;
                server.stop();
            } else if (menuIndex == 2) {
                currentState = AppState::SHARE;
                server.start();
            } else if (menuIndex == 3) {
                currentState = AppState::LANGUAGE;
                server.stop();
                langSelection = (int)I18N::getLanguage();
            }
        }

        // Removed Up/Down navigation for menuIndex as requested by user.

        // Camera frame runs in background ONLY when we are in CAMERA mode
        if (currentState == AppState::CAMERA && camera.updateFrame(&pad)) {
            FilterEngine::applyFilter(currentFilter,
                camera.getFrameBuffer(), rgbPixels, IR_WIDTH, IR_HEIGHT);
            SDL_UpdateTexture(camTexture, NULL, rgbPixels,
                IR_WIDTH * sizeof(uint32_t));
            if (firstFrame) {
                dbg_log("First camera frame rendered successfully!");
                firstFrame = false;
            }
        }
        
        if (isRecording && currentState == AppState::CAMERA) {
            uint32_t currentTicks = SDL_GetTicks();
            if (currentTicks >= nextVideoFrameTime) {
                if (videoWriter) videoWriter->addFrame(rgbPixels);
                nextVideoFrameTime = currentTicks + FRAME_DELAY_MS;
            }
        }

        // ── Input per state
        switch (currentState) {
            case AppState::CAMERA:
                if (kDown & HidNpadButton_A) {
                    if (menuIndex == 0) {
                        Gallery::savePhoto(rgbPixels, IR_WIDTH, IR_HEIGHT);
                        gallery.scanPhotos();
                    } else if (menuIndex == 1) {
                        currentState = AppState::GALLERY;
                    } else if (menuIndex == 2) {
                        currentState = AppState::SHARE;
                        server.start();
                    }
                }
                if (kDown & HidNpadButton_B) {
                    if (menuIndex == 0) {
                        if (!isRecording) {
                            time_t t = time(NULL);
                            struct tm tm = *localtime(&t);
                            char filename[256];
                            sprintf(filename, "/switch/CamNX/photos/VID_%04d%02d%02d_%02d%02d%02d.avi", 
                                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                            videoWriter = new AviWriter(filename, IR_WIDTH, IR_HEIGHT, TARGET_FPS);
                            isRecording = true;
                            recordStartTime = SDL_GetTicks();
                            nextVideoFrameTime = recordStartTime;
                        } else {
                            if (videoWriter) {
                                videoWriter->finish();
                                delete videoWriter;
                                videoWriter = nullptr;
                            }
                            isRecording = false;
                            fsdevCommitDevice("sdemmc"); // Ensure SD card metadata is flushed!
                            gallery.scanPhotos(); // Refresh gallery with new video
                        }
                    }
                }
                if (kDown & HidNpadButton_L) {
                    int m = (int)currentFilter - 1;
                    if (m < 0) m = (int)FilterMode::NUM_FILTERS - 1;
                    currentFilter = (FilterMode)m;
                }
                if (kDown & HidNpadButton_R) {
                    int m = (int)currentFilter + 1;
                    if (m >= (int)FilterMode::NUM_FILTERS) m = 0;
                    currentFilter = (FilterMode)m;
                }
                if (kDown & HidNpadButton_X) {
                    int res = camera.getResolution();
                    res++;
                    if (res > 3) res = 0;
                    camera.setResolution(res);
                }
                break;

            case AppState::GALLERY:
                gallery.handleInput(kDown);
                break;

            case AppState::SHARE:
                break;
                
            case AppState::LANGUAGE:
                if (kDown & HidNpadButton_Up) {
                    langSelection--;
                    if (langSelection < 0) langSelection = (int)Language::NUM_LANGUAGES - 1;
                }
                if (kDown & HidNpadButton_Down) {
                    langSelection++;
                    if (langSelection >= (int)Language::NUM_LANGUAGES) langSelection = 0;
                }
                if (kDown & HidNpadButton_A) {
                    I18N::setLanguage((Language)langSelection);
                    reloadFonts((Language)langSelection);
                    gallery.updateFonts(fontLg);
                }
                break;
        }

        // ── Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        switch (currentState) {
            case AppState::CAMERA: {
                SDL_Rect camDest = {PANEL_W, 0, 1280 - PANEL_W, 720};
                SDL_RenderCopy(renderer, camTexture, NULL, &camDest);
                
                if (isRecording) {
                    uint32_t now = SDL_GetTicks();
                    uint32_t elapsedSec = (now - recordStartTime) / 1000;
                    
                    if ((now / 500) % 2 == 0) {
                        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                        int cx = 1280 - 100 - 16;
                        int cy = 720 - 52 + 14; 
                        int r = 9;
                        for (int w = 0; w < r * 2; w++) {
                            for (int h = 0; h < r * 2; h++) {
                                int dx = r - w; 
                                int dy = r - h; 
                                if ((dx*dx + dy*dy) <= (r * r)) {
                                    SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
                                }
                            }
                        }
                    }
                    
                    char timeStr[16];
                    sprintf(timeStr, "%02d:%02d", elapsedSec / 60, elapsedSec % 60);
                    SDL_Color white = {255,255,255,255};
                    renderText(renderer, fontLg, timeStr, 1280 - 100, 720 - 52, white);
                }

                drawSidebar(renderer, currentState, menuIndex, currentFilter, camera.getResolution(), appLogo);
                break;
            }
            case AppState::GALLERY: {
                gallery.render();
                drawSidebar(renderer, currentState, menuIndex, currentFilter, camera.getResolution(), appLogo);
                break;
            }
            case AppState::SHARE: {
                drawShareScreen(renderer, server);
                drawSidebar(renderer, currentState, menuIndex, currentFilter, camera.getResolution(), appLogo);
                break;
            }
            case AppState::LANGUAGE: {
                drawLanguageScreen(renderer, langSelection);
                drawSidebar(renderer, currentState, menuIndex, currentFilter, camera.getResolution(), appLogo);
                break;
            }
        }


        SDL_RenderPresent(renderer);


        // Watchdog: auto-restart if server died while on SHARE screen
        if (currentState == AppState::SHARE && !server.isAlive()) {
            server.start();
        }
    }

    // ── Cleanup
    server.stop();
    camera.finalize();
    
    if (videoWriter) {
        videoWriter->finish();
        delete videoWriter;
    }

    if (appLogo) SDL_DestroyTexture(appLogo);
    if (camTexture) SDL_DestroyTexture(camTexture);
    delete[] rgbPixels;

    if (fontLg) TTF_CloseFont(fontLg);
    if (fontSm) TTF_CloseFont(fontSm);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    romfsExit();
    plExit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    nifmExit();
    socketExit();
    return 0;
}
