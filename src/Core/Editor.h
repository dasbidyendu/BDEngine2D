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
    void DrawUIPalette();
    void DrawUIViewport();
    void DrawSceneView();
	void DrawMenuBar();

    std::string currentBrowserPath = "assets";
    std::string lastPath = "";
    int browserActiveTab = 0;
    int draggedAssetIndex = -1;

    

    enum EditorMode {
        MODE_WORLD,
        MODE_UI_EDITOR
	} currentMode = MODE_WORLD;

    Entity activeCanvasId = -1;

private:
    Engine* owner;
    /*void CreateMainDockSpace();*/
};

namespace EditorSystem {
    void DrawInspector(Entity e, Registry& reg, int screenWidth, int screenHeight, Engine* engine);
    void DrawSettingsMenu(bool& open, int& activeTab, Registry& reg, Engine* engine);
    void DrawGrid(int gridSize, Camera2D camera, int screenWidth, int screenHeight, Color color);
    void DrawAssetBrowser(std::vector<AssetEntry>& assets, std::string& currentPath, int& draggedIndex);
    void DrawSpriteEditor(Entity e, Registry& reg, float xPos, float& currentY, float panelWidth, Engine* engine);
    void DrawUIInspector(Entity e, Registry& reg, float xPos, float& currentY, float panelWidth, Engine* engine);
}