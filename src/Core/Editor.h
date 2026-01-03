#pragma once
#include <string>
#include <vector>
#include "raylib.h"
#include "ECS/Entity.h"

class Engine;
class Registry;
struct AssetEntry;

class Editor {
public:
    Editor(Engine* engine) : owner(engine) {}

    void Update();
    void Render();

    std::string currentBrowserPath = "assets";
    std::string lastPath = "";
    int browserActiveTab = 0;
    int draggedAssetIndex = -1;

private:
    Engine* owner;
};

namespace EditorSystem {
    void DrawInspector(Entity e, Registry& reg, int screenWidth, int screenHeight, Engine* engine);
    void DrawSettingsMenu(bool& open, int& activeTab, Registry& reg, Engine* engine);
    void DrawGrid(int gridSize, Camera2D camera, int screenWidth, int screenHeight, Color color);
    void DrawAssetBrowser(std::vector<AssetEntry>& assets, std::string& currentPath, int& draggedIndex);
}