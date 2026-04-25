#pragma once
#include <map>
#include <string>
#include "raylib.h"
#include "TileSet.h"

class ResourceManager {
public:
	ResourceManager();
	~ResourceManager();

	void LoadTextureAsset(const std::string& name, const std::string& filePath);
	Texture2D GetTexture(const std::string& name);

    // Shaders
    void LoadShaderAsset(const std::string& name, const std::string& vsPath, const std::string& fsPath);
    void AddShader(const std::string& name, Shader shader);
    Shader GetShader(const std::string& name);
    bool HasShader(const std::string& name);

    // TileSets
    void AddTileSet(const std::string& name, const TileSet& tileSet);
    TileSet* GetTileSet(const std::string& name);

private:
	std::map<std::string, Texture2D> textures;
    std::map<std::string, Shader> shaders;
    std::map<std::string, TileSet> tileSets;
};