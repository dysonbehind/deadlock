#pragma once
#include <Windows.h>
#include <vector>

uintptr_t FindSignature(void* module, const char* signature, int offset = 0, int instructionSize = 0);