#pragma once
#include "Utils/AssetEntry.h"
#include "raylib.h"
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class AssetScanner {
public:
  static std::vector<AssetEntry> Scan(const std::string &path) {
    TraceLog(LOG_INFO, "ASSET_BROWSER: Scanning path: %s", path.c_str());
    std::vector<AssetEntry> assets;

    std::error_code ec1;
    if (!fs::exists(path, ec1) || !fs::is_directory(path, ec1)) {
      TraceLog(LOG_ERROR,
               "ASSET_BROWSER: Path does not exist or is not a directory: %s",
               path.c_str());
      return assets;
    }

    std::error_code ec2;
    for (const auto &entry : fs::directory_iterator(path, ec2)) {
      AssetEntry asset;
      asset.name = entry.path().filename().string();
      asset.path = entry.path().string();
      std::replace(asset.path.begin(), asset.path.end(), '\\', '/');

      asset.isFolder = entry.is_directory(ec2);
      asset.isTexture = false;

      if (!asset.isFolder) {
        std::string ext = entry.path().extension().string();
        asset.isTexture = (ext == ".png" || ext == ".jpg" || ext == ".bmp");
      }

      if (asset.isTexture) {
        Image img = LoadImage(asset.path.c_str());
        if (img.data != nullptr) {
          ImageResize(&img, 40, 40);
          asset.preview = LoadTextureFromImage(img);
          UnloadImage(img);
        }
      }
      assets.push_back(asset);
    }
    TraceLog(LOG_INFO, "ASSET_BROWSER: Found %d assets in %s",
             (int)assets.size(), path.c_str());
    return assets;
  }
};