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
    bool isFolder;
};

class AssetScanner {
public:
    static std::vector<AssetEntry> Scan(const std::string& path) {
        std::vector<AssetEntry> assets;

        // We use recursive_directory_iterator to find EVERYTHING in subfolders
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            AssetEntry asset;
            asset.name = entry.path().filename().string();
            asset.path = entry.path().string();

            // 1. Check if it's a directory
            asset.isFolder = entry.is_directory();

            // 2. Identify Texture files
            asset.isTexture = false;
            if (!asset.isFolder) {
                std::string ext = entry.path().extension().string();
                asset.isTexture = (ext == ".png" || ext == ".jpg" || ext == ".bmp");
            }

            // 3. Generate Preview for textures
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