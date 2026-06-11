#pragma once
#include <pthread.h>
#include <string>
#include <stdbool.h>
#include <atomic>
#include <SDL2/SDL.h>

class Server {
public:
    Server(SDL_Renderer* renderer);
    ~Server();

    void start();
    void stop();
    void refreshIP();
    void renderQR(int cx, int cy);
    std::string getIP() const { return ipAddress; }
    bool isAlive() const { return running.load(); }

private:
    SDL_Renderer* renderer;
    pthread_t thread;
    std::atomic<bool> running;
    bool threadStarted;
    int serverSocket;
    std::string ipAddress;
    SDL_Texture* qrTexture;

    static void* threadFunc(void* arg);
    void run();
    void serveClient(int clientSocket);
    void generateQR();
    std::string getLocalIP();
};
