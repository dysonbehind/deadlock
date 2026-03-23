#include "offsets.h"


namespace offset
{

    uintptr_t clientDLL = (uintptr_t)GetModuleHandleA("client.dll");

    uintptr_t CPrediction = *(uintptr_t*)FindSignature((void*)clientDLL, "48 8B 0D ? ? ? ? E8 ? ? ? ? 84 C0 74 ? 32 C0", 3, 7);

    uintptr_t CPlayerPawn = *(uintptr_t*)(CPrediction + 0xD8);

    uintptr_t CGameEntitySystem = *(uintptr_t*)FindSignature((void*)clientDLL, "4C 89 3D ? ? ? ? 4C 89 3D ? ? ? ? 48 83 C4", 3, 7);
    uintptr_t EntityList2 = CGameEntitySystem + 0x10;


    float* viewMatrix = (float*)FindSignature((void*)clientDLL, "48 8D 0D ? ? ? ? 48 C1 E0 06 48 03 C1 C3", 3, 7);
}