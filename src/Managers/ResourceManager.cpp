#include "ResourceManager.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

ResourceManager::ResourceManager(){}

ResourceManager::~ResourceManager() {
	for (auto& entry : textures) {
		UnloadTexture(entry.second);
	}
	textures.clear();

    for (auto& entry : shaders) {
        UnloadShader(entry.second);
    }
    shaders.clear();
}

void ResourceManager::LoadTextureAsset(const std::string& name, const std::string& filePath) {
	if (textures.find(name) != textures.end()) {
		std::cout << "Texture already loaded : " << name << std::endl;
		return;
	}

	Texture2D tex = LoadTexture(filePath.c_str());

	if (tex.id == 0) {
		if (!std::filesystem::exists(filePath)) {
			std::cout << "ERROR: File does not exist at path: " << filePath << std::endl;
		}
		std::cout << "Failed to Load Texture : " << filePath << std::endl;
	}
	else {
		textures[name] = tex;
		std::cout << "Loaded Texture: " << name << std::endl;
	}
}

Texture2D ResourceManager::GetTexture(const std::string& name) {
	if (textures.find(name) != textures.end()) {
		return textures[name];
	}

	std::cout << "Texture Not Found: " << name << std::endl;
	return { 0 };
}

void ResourceManager::LoadShaderAsset(const std::string& name, const std::string& vsPath, const std::string& fsPath) {
    if (shaders.find(name) != shaders.end()) {
        std::cout << "Shader already loaded: " << name << std::endl;
        return;
    }

    const char* vs = vsPath.empty() ? nullptr : vsPath.c_str();
    const char* fs = fsPath.empty() ? nullptr : fsPath.c_str();
    
    Shader sh = LoadShader(vs, fs);
    if (sh.id == 0) {
        std::cout << "Failed to Load Shader: " << name << std::endl;
    } else {
        shaders[name] = sh;
        std::cout << "Loaded Shader: " << name << std::endl;
    }
}

void ResourceManager::AddShader(const std::string& name, Shader shader) {
    if (shaders.find(name) != shaders.end()) {
        UnloadShader(shaders[name]); // Overwrite existing
    }
    shaders[name] = shader;
    std::cout << "Added/Updated Shader: " << name << std::endl;
}

Shader ResourceManager::GetShader(const std::string& name) {
    if (shaders.find(name) != shaders.end()) {
        return shaders[name];
    }
    std::cout << "Shader Not Found: " << name << std::endl;
    return { 0 };
}

bool ResourceManager::HasShader(const std::string& name) {
    return shaders.find(name) != shaders.end();
}

void ResourceManager::AddTileSet(const std::string& name, const TileSet& tileSet) {
    tileSets[name] = tileSet;
}

TileSet* ResourceManager::GetTileSet(const std::string& name) {
    if (tileSets.find(name) != tileSets.end()) {
        return &tileSets[name];
    }
    
    // Attempt dynamic load from disk
    if (std::filesystem::exists(name)) {
        TileSet newSet;
        newSet.Load(name);
        LoadTextureAsset(newSet.name, newSet.texturePath);
        newSet.texture = GetTexture(newSet.name);
        tileSets[name] = newSet;
        return &tileSets[name];
    }
    
    return nullptr;
}

void TileSet::Save(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return;
    
    file << name << "\n";
    file << texturePath << "\n";
    file << tileSize << "\n";
    file << sourceRects.size() << "\n";
    
    for (size_t i = 0; i < sourceRects.size(); ++i) {
        const auto& r = sourceRects[i];
        file << r.x << " " << r.y << " " << r.width << " " << r.height << "\n";
        
        if (i < tileConfigs.size()) {
            const auto& cfg = tileConfigs[i];
            file << (cfg.isRuleTile ? 1 : 0) << " " << cfg.rules.size() << "\n";
            for (const auto& rule : cfg.rules) {
                file << rule.outputIndex;
                for (int n = 0; n < 8; n++) {
                    file << " " << (int)rule.neighbors[n];
                }
                file << "\n";
            }
        } else {
            file << "0 0\n";
        }
    }
}

void TileSet::Load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;
    
    std::getline(file, name);
    std::getline(file, texturePath);
    
    file >> tileSize;
    size_t rectCount;
    file >> rectCount;
    
    sourceRects.clear();
    tileConfigs.clear();
    
    for (size_t i = 0; i < rectCount; ++i) {
        Rectangle r;
        file >> r.x >> r.y >> r.width >> r.height;
        sourceRects.push_back(r);
        
        TileConfig cfg;
        cfg.index = (int)i;
        
        int isRule = 0, ruleCount = 0;
        if (file >> isRule >> ruleCount) {
            cfg.isRuleTile = (isRule == 1);
            for(int r = 0; r < ruleCount; r++) {
                TileRule tr;
                file >> tr.outputIndex;
                for(int n = 0; n < 8; n++) {
                    int cond;
                    file >> cond;
                    tr.neighbors[n] = (NeighborCondition)cond;
                }
                cfg.rules.push_back(tr);
            }
        }
        
        tileConfigs.push_back(cfg);
    }
}