#include "memory.h"
#include <iostream>

bool MemoryReader::Initialize() {
    HWND window = FindWindowA(NULL, "WSS 64  ");
    if (!window) {
        MessageBoxA(NULL, "Failed to find WSS 64 window", "Debug", MB_OK);
        return false;
    }

    DWORD processId;
    GetWindowThreadProcessId(window, &processId);
    if (!processId) {
        MessageBoxA(NULL, "Failed to get process ID", "Debug", MB_OK);
        return false;
    }

    processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (!processHandle) {
        MessageBoxA(NULL, "Failed to open process", "Debug", MB_OK);
        return false;
    }

    moduleBase = GetModuleBaseAddress(processId, L"WSS-Win64-Shipping.exe");
    if (!moduleBase) {
        MessageBoxA(NULL, "Failed to find game executable", "Debug", MB_OK);
        return false;
    }

    if (!FindUWorld()) {
        MessageBoxA(NULL, "Failed to find UWorld", "Debug", MB_OK);
        return false;
    }

    char debug[256];
    sprintf_s(debug, "ModuleBase: 0x%llX\nUWorld: 0x%llX",
        moduleBase, uWorld);
    MessageBoxA(NULL, debug, "Debug", MB_OK);

    return true;
}

bool MemoryReader::FindUWorld() {
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t addr = moduleBase;

    while (VirtualQueryEx(processHandle, (LPVOID)addr, &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect & PAGE_EXECUTE_READ || mbi.Protect & PAGE_EXECUTE_READWRITE)) {

            std::vector<uint8_t> buffer(mbi.RegionSize);
            SIZE_T bytesRead;

            if (ReadProcessMemory(processHandle, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {
                for (size_t i = 0; i < buffer.size() - 10; i++) {
                    if (buffer[i] == 0x48 && buffer[i + 1] == 0x8B && buffer[i + 2] == 0x0D &&
                        buffer[i + 7] == 0x48 && buffer[i + 8] == 0x85 && buffer[i + 9] == 0xC9) {

                        uintptr_t instructionAddr = (uintptr_t)mbi.BaseAddress + i;
                        int32_t offset = *(int32_t*)(buffer.data() + i + 3);
                        uWorld = instructionAddr + offset + 7;
                        return true;
                    }
                }
            }
        }
        addr += mbi.RegionSize;
    }
    return false;
}

uintptr_t MemoryReader::GetModuleBaseAddress(DWORD processId, const wchar_t* moduleName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    MODULEENTRY32W moduleEntry;
    moduleEntry.dwSize = sizeof(moduleEntry);

    if (Module32FirstW(snapshot, &moduleEntry)) {
        do {
            if (_wcsicmp(moduleEntry.szModule, moduleName) == 0) {
                CloseHandle(snapshot);
                return (uintptr_t)moduleEntry.modBaseAddr;
            }
        } while (Module32NextW(snapshot, &moduleEntry));
    }

    CloseHandle(snapshot);
    return 0;
}

std::vector<GameObject> MemoryReader::GetObjects() {
    std::vector<GameObject> objects;
    char debug[256];

    sprintf_s(debug, "UWorld: 0x%llX", uWorld);
    MessageBoxA(NULL, debug, "Debug - GetObjects", MB_OK);

    uintptr_t uLevel;
    ReadProcessMemory(processHandle, (LPCVOID)(uWorld + 0x30), &uLevel, sizeof(uLevel), nullptr);

    sprintf_s(debug, "ULevel: 0x%llX", uLevel);
    MessageBoxA(NULL, debug, "Debug - GetObjects", MB_OK);

    uintptr_t actorArray;
    int32_t actorCount;
    ReadProcessMemory(processHandle, (LPCVOID)(uLevel + 0x98), &actorArray, sizeof(actorArray), nullptr);
    ReadProcessMemory(processHandle, (LPCVOID)(uLevel + 0xA0), &actorCount, sizeof(actorCount), nullptr);

    sprintf_s(debug, "ActorArray: 0x%llX\nActorCount: %d", actorArray, actorCount);
    MessageBoxA(NULL, debug, "Debug - GetObjects", MB_OK);

    for (int i = 0; i < actorCount && i < 1000; i++) {
        uintptr_t actor;
        ReadProcessMemory(processHandle, (LPCVOID)(actorArray + i * 8), &actor, sizeof(actor), nullptr);

        if (actor != 0) {
            char name[256] = { 0 };
            uintptr_t namePtr;
            ReadProcessMemory(processHandle, (LPCVOID)(actor + 0x18), &namePtr, sizeof(namePtr), nullptr);
            ReadProcessMemory(processHandle, (LPCVOID)namePtr, name, sizeof(name), nullptr);

            if (strstr(name, "BP_Survivor") || strstr(name, "BP_Animal")) {
                GameObject obj;

                uintptr_t rootComponent;
                ReadProcessMemory(processHandle, (LPCVOID)(actor + 0x130), &rootComponent, sizeof(rootComponent), nullptr);

                ReadProcessMemory(processHandle, (LPCVOID)(rootComponent + 0x11C), &obj.position, sizeof(XMFLOAT3), nullptr);

                obj.dimensions = XMFLOAT3(100.0f, 100.0f, 200.0f);
                obj.isValid = true;

                objects.push_back(obj);
            }
        }
    }

    return objects;
}

XMMATRIX MemoryReader::GetViewMatrix() {
    uintptr_t gameInstance, playerController, playerCameraManager;
    ReadProcessMemory(processHandle, (LPCVOID)(uWorld + 0x180), &gameInstance, sizeof(gameInstance), nullptr);
    ReadProcessMemory(processHandle, (LPCVOID)(gameInstance + 0x38), &playerController, sizeof(playerController), nullptr);
    ReadProcessMemory(processHandle, (LPCVOID)(playerController + 0x2B8), &playerCameraManager, sizeof(playerCameraManager), nullptr);

    XMMATRIX viewMatrix;
    ReadProcessMemory(processHandle, (LPCVOID)(playerCameraManager + 0x1F0), &viewMatrix, sizeof(XMMATRIX), nullptr);

    return viewMatrix;
}

XMMATRIX MemoryReader::GetProjectionMatrix() {
    float fovAngle = 90.0f;
    float width = 1920.0f;
    float height = 1080.0f;

    return XMMatrixPerspectiveFovLH(
        (fovAngle * 0.017453292f),
        width / height,
        0.1f,
        1000.0f
    );
}