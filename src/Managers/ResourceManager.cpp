#include "ResourceManager.h"
#include <iostream>
#include <filesystem>
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