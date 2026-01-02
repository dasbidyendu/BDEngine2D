#pragma once
#include <map>
#include <string>
#include "raylib.h"

class ResourceManager {
public:
	ResourceManager();
	~ResourceManager();

	void LoadTextureAsset(const std::string& name, const std::string& filePath);
	Texture2D GetTexture(const std::string& name);

private:
	std::map<std::string, Texture2D> textures;
};