#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

struct GameObject {
    XMFLOAT3 position;
    XMFLOAT3 dimensions;
    bool isValid;
};

class MemoryReader {
public:
    bool Initialize();
    std::vector<GameObject> GetObjects();
    XMMATRIX GetViewMatrix();
    XMMATRIX GetProjectionMatrix();

private:
    HANDLE processHandle;
    uintptr_t moduleBase;
    uintptr_t unityPlayerBase;
    uintptr_t objectListPtr = 0;
    uintptr_t uWorld = 0;

    uintptr_t GetModuleBaseAddress(DWORD processId, const wchar_t* moduleName);
    bool FindUWorld();

    template<typename T>
    T Read(uintptr_t address) {
        T value{};
        ReadProcessMemory(processHandle, (LPCVOID)address, &value, sizeof(T), nullptr);
        return value;
    }
};