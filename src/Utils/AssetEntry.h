#pragma once
#include <string>
#include "raylib.h"

struct AssetEntry {
    std::string name;
    std::string path;
    bool isFolder;
    bool isTexture;
    Texture2D preview;
};
