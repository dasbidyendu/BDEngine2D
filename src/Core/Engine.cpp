#define _CRT_SECURE_NO_WARNINGS
#include "raylib.h"
#include "Engine.h"
#include "Editor.h"
#include <fstream>
#include <sstream>
#include "Managers/SceneManager.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

Engine::Engine(int width, int height, const std::string& title)
    :screenWidth(width), screenHeight(height), windowTitle(title) {

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, windowTitle.c_str());
    int monitor = GetCurrentMonitor();
    if (!IsWindowFullscreen()) {
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        ToggleFullscreen();
    }

    SetTargetFPS(0);
    isRunning = true;

    TraceLog(LOG_INFO, "Window Ready");
    LoadConfig();

    camera.target = { 0.0f, 0.0f };
    camera.offset = { 0.0f, 0.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);

    registry = std::make_unique<Registry>();

    try {
        TraceLog(LOG_INFO, "SYSTEM: Initializing Script Engine...");
        scriptEngine = std::make_unique<ScriptEngine>();
        // Only init if pointer is valid
        if (scriptEngine) {
            scriptEngine->Init(*registry,&camera);
            TraceLog(LOG_INFO, "SYSTEM: Script Engine Ready.");
        }
    }
    catch (...) {
        TraceLog(LOG_ERROR, "CRITICAL: Script Engine failed to initialize!");
    }

    editor = std::make_unique<Editor>(this);

    isRunning = true;
}

Engine::~Engine() {
    SaveConfig();
    CloseWindow();
}

void Engine::LoadConfig() {
    TraceLog(LOG_INFO, "CONFIG: Loading EditorConfig.ini...");
    std::ifstream file("EditorConfig.ini");

    int targetWidth = 1280;
    int targetHeight = 720;

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream is_line(line);
            std::string key, value;
            if (std::getline(is_line, key, '=') && std::getline(is_line, value)) {
                if (key == "LastTheme") lastThemePath = value;
                if (key == "LastFont") lastFontPath = value;
                if (key == "ShowGrid") showGrid = (value == "true");
                if (key == "GridSize") gridSize = std::stoi(value);
                if (key == "ScreenWidth") targetWidth = std::stoi(value);
                if (key == "ScreenHeight") targetHeight = std::stoi(value);
            }
        }
        file.close();
    }

    screenWidth = targetWidth;
    screenHeight = targetHeight;
    SetWindowSize(screenWidth, screenHeight);

    ScanFonts();
    LoadEngineFont(lastFontPath);

    ScanThemes();
	ApplyTheme(lastThemePath);
}

void Engine::SaveConfig() {
    std::ofstream file("EditorConfig.ini");
    if (file.is_open()) {
        file << "ScreenWidth=" << GetScreenWidth() << "\n";
        file << "ScreenHeight=" << GetScreenHeight() << "\n";
        file << "LastTheme=" << lastThemePath << "\n";
        file << "LastFont=" << lastFontPath << "\n";
        file << "ShowGrid=" << (showGrid ? "true" : "false") << "\n";
        file << "GridSize=" << gridSize << "\n";
        file.close();
    }
}

void Engine::Run() {
    TraceLog(LOG_INFO, "ENGINE: Entering Main Loop");
    while (!WindowShouldClose() && isRunning) {
        Update();
        Render();
    }
}

void Engine::InitGame() {
    Entity gameEntity = registry->CreateEntity();

	ScriptComponent sc;
	sc.scriptPaths.push_back("assets/scripts/echo.lua");
	registry->AddComponent(gameEntity, sc);

    editorAssets = AssetScanner::Scan("assets");
    ScanThemes();

    for (const auto& asset : editorAssets) {
        if (asset.isTexture) {
            assets.LoadTextureAsset(asset.name, asset.path);
        }
    }

    SceneManager::LoadScene("assets/scenes/main.scene", *registry, this);
}

void Engine::Update() {
	if (currentState == EngineState::STARTUP) {
        if (activeToast.active) {
            activeToast.timeRemaining -= GetFrameTime();

            if (activeToast.timeRemaining < 1.0f) {
                activeToast.opacity = activeToast.timeRemaining;
            }
            else {
                activeToast.opacity = 1.0f;
            }

            if (activeToast.timeRemaining <= 0) {
                activeToast.active = false;
            }
        }
        return;
    }
    double startTime = GetTime();

    if (IsWindowResized() || IsWindowFullscreen()) {
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
    }

    stats.frameCount++;

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
        SceneManager::SaveScene("assets/scenes/main.scene", *registry);
        TraceLog(LOG_INFO, "SYSTEM: Scene saved to assets/scenes/main.scene");
    }

    // INPUT FOCUS KILL-SWITCH
    if (activeControlId != 0) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
            activeControlId = 0;
        }
    }

    // GLOBAL OVERLAY TOGGLES
    if (IsKeyPressed(KEY_F1)) showSettings = !showSettings;
    if (IsKeyPressed(KEY_TAB)) isEditorMode = !isEditorMode;
    if (IsKeyPressed(KEY_F11) || (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER))) ToggleFullscreen();
    if (IsKeyPressed(KEY_F2)) stats.Toggle();

    // V-Sync Toggle
    if (IsKeyPressed(KEY_F3)) {
        if (IsWindowState(FLAG_VSYNC_HINT)) {
            ClearWindowState(FLAG_VSYNC_HINT);
            SetTargetFPS(0);
        }
        else {
            SetWindowState(FLAG_VSYNC_HINT);
        }
    }

    //MAIN LOGIC
    if (!showSettings) {
        if (isEditorMode) {
            editor->Update();

            
            bool isTyping = (activeControlId != 0);

            if (!isTyping && GetMousePosition().x > 250 && GetMousePosition().x < (screenWidth - 300)) {

                // Right-Click Pan
                if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                    Vector2 delta = GetMouseDelta();
                    camera.target.x -= delta.x / camera.zoom;
                    camera.target.y -= delta.y / camera.zoom;
                }

                // Zoom Logic (Centered on Mouse)
                float wheel = GetMouseWheelMove();
                if (wheel != 0) {
                    Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
                    camera.offset = GetMousePosition();
                    camera.target = mouseWorldPos;
                    camera.zoom += wheel * zoomSensitivity;
                    camera.zoom = Clamp(camera.zoom, 0.1f, 5.0f);
                }
            }
        }
        else {
            // PLAY MODE LOGIC
            InputSystem::Update(*registry);
            ControlSystem::Update(*registry);

            float dt = GetFrameTime();
            MovementSystem::Update(*registry, dt);

            if (scriptEngine) {
                for (Entity i = 0; i < MAX_ENTITIES; i++) {
                    if (registry->HasComponent(i, COMP_SCRIPT)) {
                        auto& script = registry->scripts[i];
                        for (const std::string& path : script.scriptPaths) {
                            if (!path.empty()) {
                                scriptEngine->Execute(i, path, dt);
                            }
                        }
                    }
                }
            }
        }
    }

    double endTime = GetTime();
    stats.logicTime = (float)(endTime - startTime) * 1000.0f;

    if (activeToast.active) {
        activeToast.timeRemaining -= GetFrameTime();

        if (activeToast.timeRemaining < 1.0f) {
            activeToast.opacity = activeToast.timeRemaining;
        }
        else {
            activeToast.opacity = 1.0f;
        }

        if (activeToast.timeRemaining <= 0) {
            activeToast.active = false;
        }
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
        SceneManager::SaveScene(currentScenePath, *registry);
        TraceLog(LOG_INFO, "SYSTEM: Scene saved to %s", currentScenePath.c_str());
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_L)) {
		SceneManager::LoadScene(currentScenePath, *registry, this);
    }
}

void Engine::Render() {
    BeginDrawing();
    ClearBackground(currentTheme.background);
    if (currentState == EngineState::STARTUP) {
		DrawStartupScreen();
        DrawFileBrowser();
    }
    else {
        BeginMode2D(camera);
        if (isEditorMode && showGrid) {
            EditorSystem::DrawGrid(gridSize, camera, screenWidth, screenHeight, gridColor);
        }

        if (isEditorMode && selectedEntity != -1 && registry->HasComponent(selectedEntity, COMP_TRANSFORM)) {
            auto& t = registry->transforms[selectedEntity];
            auto& s = registry->sprites[selectedEntity];
            Rectangle outline = { t.position.x - 2, t.position.y - 2,
                                 (s.texture.width * t.scale.x) + 4, (s.texture.height * t.scale.y) + 4 };
            DrawRectangleLinesEx(outline, 2.0f / camera.zoom, ORANGE);
        }

        RenderSystem::Draw(*registry);
        EndMode2D();

        UISystem::Draw(*registry);

        if (isEditorMode) {
            editor->Render();
        }

        DebugSystem::Draw(*registry, stats, GetScreenWidth());

    }
    
	DrawToast();

    EndDrawing();
}

void Engine::DrawStartupScreen(){
    int centerX = GetScreenWidth() / 2 - 100;
    int centerY = GetScreenHeight() / 2 - 50;

    DrawText("BDENGINE2D", centerX - 20, centerY - 60, 30, currentTheme.accentColor);

    // OPEN SCENE
    if (GuiButton({ (float)centerX, (float)centerY, 200, 40 }, "Open Scene")) {
        RefreshSceneList();
            showFileBrowser = true;
			DrawFileBrowser();
			ShowToast("Select a scene to open.");
    }
    // NEW SCENE 
    if (GuiButton({ (float)centerX, (float)centerY + 50, 200, 40 }, "New Scene")) {
        try {
            registry = std::make_unique<Registry>();
            if (!registry) throw std::runtime_error("Failed to allocate Registry!");
            currentScenePath = "assets/scenes/new_level.scene";
            currentState = EngineState::EDITOR;
        }
        catch (const std::exception& e) {
            ShowToast("Critical: Could not create new scene.");
        }
    }

}

void Engine::RefreshSceneList() {
    sceneFiles.clear();
    if (DirectoryExists("assets/scenes")) {
        FilePathList files = LoadDirectoryFiles("assets/scenes");
        for (unsigned int i = 0; i < files.count; i++) {
            if (IsFileExtension(files.paths[i], ".scene")) {
                sceneFiles.push_back(files.paths[i]);
            }
        }
        UnloadDirectoryFiles(files);
    }
}

void Engine::DrawFileBrowser() {
    if (!showFileBrowser) return;

    float width = 400;
    float height = 300;
    float x = (GetScreenWidth() - width) / 2;
    float y = (GetScreenHeight() - height) / 2;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.4f));

    if (GuiWindowBox({ x, y, width, height }, "SELECT SCENE")) {
        showFileBrowser = false;
    }

    float itemHeight = 35;
    for (size_t i = 0; i < sceneFiles.size(); i++) {
        std::string fileName = GetFileName(sceneFiles[i].c_str());

        if (GuiButton({ x + 10, y + 40 + (i * (itemHeight + 5)), width - 20, itemHeight }, fileName.c_str())) {
            try {
                currentScenePath = sceneFiles[i];
                currentState = EngineState::EDITOR;
                showFileBrowser = false;
                SceneManager::LoadScene(currentScenePath, *registry, this);
            }
            catch (const std::exception& e) {
                ShowToast(e.what());
            }
        }
    }
}

void Engine::ShowToast(const std::string& msg) {
    activeToast.message = msg;
    activeToast.timeRemaining = 4.0f;
    activeToast.active = true;
    activeToast.opacity = 1.0f;
}

void Engine::DrawToast() {
    if (!activeToast.active) return;

    float alpha = (activeToast.timeRemaining < 1.0f) ? activeToast.timeRemaining : 1.0f;

    int tw = 320;
    int th = 50;
    float posX = GetScreenWidth() - tw - 20;
    float posY = GetScreenHeight() - th - 20;

    DrawRectangleRounded({ posX, posY, (float)tw, (float)th }, 0.3f, 6, ColorAlpha(currentTheme.panelBG, alpha * 0.95f));
    DrawRectangleRoundedLines({ posX, posY, (float)tw, (float)th }, 0.3f, 6, ColorAlpha(RED, alpha));

    DrawText("!", (int)posX + 15, (int)posY + 15, 20, ColorAlpha(RED, alpha));
    DrawText(activeToast.message.c_str(), (int)posX + 40, (int)posY + 18, 14, ColorAlpha(WHITE, alpha));
}

void Engine::ScanThemes() {
    TraceLog(LOG_INFO, "THEME_SYSTEM: Scanning assets/themes/ for .txt files...");

    themeFiles.clear();
    FilePathList files = LoadDirectoryFiles("assets/themes");

    if (files.count == 0) {
        TraceLog(LOG_WARNING, "THEME_SYSTEM: No files found in assets/themes/");
    }

    for (unsigned int i = 0; i < files.count; i++) {
        if (IsFileExtension(files.paths[i], ".txt")) {
            themeFiles.push_back(files.paths[i]);
            TraceLog(LOG_INFO, "THEME_SYSTEM: Found theme file: %s", files.paths[i]);
        }
    }

    TraceLog(LOG_INFO, "THEME_SYSTEM: Scan complete. Total themes found: %d", themeFiles.size());
    UnloadDirectoryFiles(files);
}

void Engine::ScanFonts() {
    fontFiles.clear();
    FilePathList files = LoadDirectoryFiles("assets/fonts");
    for (unsigned int i = 0; i < files.count; i++) {
        if (IsFileExtension(files.paths[i], ".ttf") || IsFileExtension(files.paths[i], ".otf")) {
            fontFiles.push_back(files.paths[i]);
            TraceLog(LOG_INFO, "FONT_SYSTEM: Found font: %s", files.paths[i]);
        }
    }
    UnloadDirectoryFiles(files);
}

void Engine::LoadEngineFont(const std::string& path) {
    if (!FileExists(path.c_str())) {
        TraceLog(LOG_WARNING, "FONT_SYSTEM: Font file not found: %s", path.c_str());
        return;
    }

    if (engineFont.texture.id != 0) UnloadFont(engineFont);

    engineFont = LoadFontEx(path.c_str(), 24, 0, 250);

    SetTextureFilter(engineFont.texture, TEXTURE_FILTER_BILINEAR);

    GuiSetFont(engineFont);

    //ApplyTheme(lastThemePath);

    lastFontPath = path;

    TraceLog(LOG_INFO, "FONT_SYSTEM: Successfully loaded font: %s", path.c_str());
}

void Engine::ApplyTheme(const std::string& filePath) {
    TraceLog(LOG_INFO, "THEME_SYSTEM: Attempting to apply theme: %s", filePath.c_str());

    char* text = LoadFileText(filePath.c_str());
    if (text == NULL) {
        TraceLog(LOG_ERROR, "THEME_SYSTEM: Failed to load theme file text!");
        return;
    }

    std::string content(text);
    UnloadFileText(text);

    auto parseColor = [&](const std::string& key) -> Color {
        size_t pos = content.find(key + ": ");
        if (pos != std::string::npos) {
            int r, g, b, a;
            const char* start = content.c_str() + pos + key.length() + 2;
            if (sscanf(start, "%d, %d, %d, %d", &r, &g, &b, &a) == 4) {
                TraceLog(LOG_INFO, "THEME_SYSTEM: Parsed %s -> R:%d G:%d B:%d A:%d", key.c_str(), r, g, b, a);
                return { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
            }
        }
        TraceLog(LOG_WARNING, "THEME_SYSTEM: Could not parse key: %s", key.c_str());
        return MAGENTA;
        };

    currentTheme.background = parseColor("bg");
    currentTheme.panelBG = parseColor("panel");
    currentTheme.gridColor = parseColor("grid");
    currentTheme.accentColor = parseColor("accent");

    size_t fontPos = content.find("fontSize: ");
    if (fontPos != std::string::npos) {
        int fSize = 0;
        sscanf(content.c_str() + fontPos + 10, "%d", &fSize);
        currentTheme.fontSize = fSize;
        GuiSetStyle(DEFAULT, TEXT_SIZE, fSize);
        TraceLog(LOG_INFO, "THEME_SYSTEM: Set FontSize to %d", fSize);
    }

    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(currentTheme.panelBG));
    GuiSetStyle(DEFAULT, LINE_COLOR, ColorToInt(currentTheme.accentColor));
    GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(currentTheme.accentColor));

    lastThemePath = filePath;
    TraceLog(LOG_INFO, "THEME_SYSTEM: Theme '%s' applied and saved to config.", filePath.c_str());
}