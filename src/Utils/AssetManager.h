#pragma once
#include <filesystem>
#include <vector>
#include "raylib.h"
namespace fs = std::filesystem;

struct AssetEntry {
    std::string name;
    std::string path;
    Texture2D preview; 
    bool isTexture;
};

class AssetScanner {
public:
    static std::vector<AssetEntry> Scan(const std::string& path) {
        std::vector<AssetEntry> assets;
        for (const auto& entry : fs::directory_iterator(path)) {
            AssetEntry asset;
            asset.name = entry.path().filename().string();
            asset.path = entry.path().string();
            asset.isTexture = (entry.path().extension() == ".png" || entry.path().extension() == ".jpg");

            if (asset.isTexture) {
                Image img = LoadImage(asset.path.c_str());
                ImageResize(&img, 40, 40);
                asset.preview = LoadTextureFromImage(img);
                UnloadImage(img);
            }
            assets.push_back(asset);
        }
        return assets;
    }
};