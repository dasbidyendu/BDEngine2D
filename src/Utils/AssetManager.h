#pragma once
#include <filesystem>
#include <vector>
#include "raylib.h"
#include "Utils/AssetEntry.h"

namespace fs = std::filesystem;

class AssetScanner {
public:
    static std::vector<AssetEntry> Scan(const std::string& path) {
        std::vector<AssetEntry> assets;

        if (!fs::exists(path) || !fs::is_directory(path)) return assets;

        for (const auto& entry : fs::directory_iterator(path)) {
            AssetEntry asset;
            asset.name = entry.path().filename().string();
            asset.path = entry.path().string();
            std::replace(asset.path.begin(), asset.path.end(), '\\', '/');

            asset.isFolder = entry.is_directory();
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
        return assets;
    }
};