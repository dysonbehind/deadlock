#include "scanner.h"

uintptr_t FindSignature(void* module, const char* signature, int offset, int instructionSize)
{
    static auto patternToByte = [](const char* pattern)
        {
            std::vector<int> bytes;
            const char* current = pattern;

            while (*current)
            {
                if (*current == '?')
                {
                    ++current;
                    if (*current == '?')
                        ++current;
                    bytes.push_back(-1);
                }
                else
                {
                    bytes.push_back(strtoul(current, const_cast<char**>(&current), 16));
                }
                ++current;
            }

            return bytes;
        };

    auto dos = (PIMAGE_DOS_HEADER)module;
    auto nt = (PIMAGE_NT_HEADERS)((uint8_t*)module + dos->e_lfanew);

    size_t size = nt->OptionalHeader.SizeOfImage;

    auto patternBytes = patternToByte(signature);

    uint8_t* scanBytes = reinterpret_cast<uint8_t*>(module);

    size_t s = patternBytes.size();
    int* d = patternBytes.data();

    uintptr_t address = 0;

    for (size_t i = 0; i < size - s; i++)
    {
        bool found = true;

        for (size_t j = 0; j < s; j++)
        {
            if (scanBytes[i + j] != d[j] && d[j] != -1)
            {
                found = false;
                break;
            }
        }

        if (found)
        {
            address = (uintptr_t)&scanBytes[i];
            break;
        }
    }

    if (!address)
        return 0;

    // если нужно резолвить rel32
    if (offset && instructionSize)
    {
        int32_t relative = *(int32_t*)(address + offset);
        return address + instructionSize + relative;
    }

    return address;
}