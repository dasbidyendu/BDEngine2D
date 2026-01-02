#include "ResourceManager.h"
#include <iostream>

ResourceManager::ResourceManager(){}

ResourceManager::~ResourceManager() {
	for (auto& entry : textures) {
		UnloadTexture(entry.second);
	}
	textures.clear();
}

void ResourceManager::LoadTextureAsset(const std::string& name, const std::string& filePath) {
	if (textures.find(name) != textures.end()) {
		std::cout << "Texture already loaded : " << name << std::endl;
		return;
	}

	Texture2D tex = LoadTexture(filePath.c_str());

	if (tex.id == 0) {
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