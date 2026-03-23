#pragma once
#include <string>

namespace Config
{
    bool Load(const std::string& path);
    bool Save(const std::string& path);
}
