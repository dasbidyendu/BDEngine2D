#pragma once
#include "raylib.h"
#include <string>
#include <vector>
#include <memory>
#include "Profiler.h"
#include "Managers/ResourceManager.h"
#include "ECS/Systems.h"
#include "Managers/ScriptSystem.h"
#include "ECS/UISystem.h"
#include "raymath.h"
#include "Utils/AssetEntry.h"
#include "Core/Physics.h"

class Editor;

class Engine {
public:
    Engine(int width, int height, const std::string& title);
    ~Engine();

    void InitGame();
    void Run();
    void InitScripting();

    void ApplyTheme(const std::string& themeName);
    void SaveConfig();
    void LoadConfig();
    void LoadEngineFont(const std::string& path);

    Camera2D& GetCamera() { return camera; }

    bool IsMouseOverUI = false;
    bool isEditorMode = true;
    bool showSettings = false;
    
    int activeControlId = 0;

    std::unique_ptr<Editor> editor;

    std::vector<AssetEntry> editorAssets;
    Entity selectedEntity = -1;
    int selectedAssetIndex = -1;
    int settingsActiveTab = 0;

    ResourceManager assets;
    std::unique_ptr<Registry> registry;

    std::vector<std::string> themeFiles;
    std::vector<std::string> fontFiles;
    std::string lastFontPath = "assets/fonts/Mecha.ttf";

    std::unique_ptr<ScriptEngine> scriptEngine;
    DebugStats stats;

private:
    void Update();
    void Render();

    int screenWidth;
    int screenHeight;
    const std::string windowTitle;
    bool isRunning;

    bool showGrid = true;
    int gridSize = 32;
    Color gridColor = { 200, 200, 200, 40 };

    Camera2D camera = { 0 };
    float zoomSensitivity = 0.1f;

    struct EngineTheme {
        std::string name;
        Color background;
        Color panelBG;
        Color gridColor;
        Color accentColor;
        int fontSize;
        unsigned int borderId;
        unsigned int textId;
    } currentTheme;

    std::string lastThemePath = "assets/themes/dark-gold.txt";
    void ScanThemes();

    Font engineFont;
    void ScanFonts();

	PhysicsSystem physicsSystem;
    SpatialHashGrid physicsGrid{ 2000, 2000, 100 };

	float physicsTimeAccumulator = 0.0f;
};