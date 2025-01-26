/*
* File: main.cpp
* Main entry point for Winter Survival ESP
*/

#include "memory.h"
#include "overlay.h"
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    MessageBoxA(NULL, "Starting...", "Debug", MB_OK);

    MemoryReader memory;
    if (!memory.Initialize()) {
        MessageBoxA(NULL, "Failed to initialize memory reader", "Debug", MB_OK);
        return 1;
    }

    MessageBoxA(NULL, "Memory reader initialized", "Debug", MB_OK);

    Overlay overlay;
    if (!overlay.Initialize()) {
        MessageBoxA(NULL, "Failed to initialize overlay", "Debug", MB_OK);
        return 1;
    }

    MessageBoxA(NULL, "Overlay initialized", "Debug", MB_OK);
    MessageBoxA(NULL, "Running... Press END to exit", "Debug", MB_OK);

    while (true) {
        if (GetAsyncKeyState(VK_END) & 1) {
            MessageBoxA(NULL, "Exiting", "Debug", MB_OK);
            break;
        }

        overlay.BeginScene();
        overlay.Render(memory);
        overlay.EndScene();

        Sleep(16); // ~60 FPS
    }

    return 0;
}