#pragma once
#include "raylib.h"
#include <string>


struct AssetEntry {
  std::string name;
  std::string path;
  bool isFolder = false;
  bool isTexture = false;
  Texture2D preview = {0};
};
