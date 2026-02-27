#define _CRT_SECURE_NO_WARNINGS
#include "Engine.h"
#include "Editor.h"
#include "Managers/SceneManager.h"
#include "raylib.h"
#include <fstream>
#include <sstream>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

Engine::Engine(int width, int height, const std::string &title)
    : screenWidth(width), screenHeight(height), windowTitle(title) {

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(screenWidth, screenHeight, windowTitle.c_str());
  viewportTarget = LoadRenderTexture(screenWidth, screenHeight);
  gameTarget = LoadRenderTexture(screenWidth, screenHeight);
  /*int monitor = GetCurrentMonitor();
  if (!IsWindowFullscreen()) {
      SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
      ToggleFullscreen();
  }*/
#ifndef BD_SHIPPING
  rlImGuiSetup(true);
  // Enable Docking
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Disabled:
  // Incompatible with rlImGui
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#endif

  SetTargetFPS(0);
  isRunning = true;

  TraceLog(LOG_INFO, "Window Ready");
  LoadConfig();

  camera.target = {0.0f, 0.0f};
  camera.offset = {0.0f, 0.0f};
  camera.rotation = 0.0f;
  camera.zoom = 1.0f;

  GuiSetStyle(DEFAULT, TEXT_SIZE, 16);

  registry = std::make_unique<Registry>();

  try {
    TraceLog(LOG_INFO, "SYSTEM: Initializing Script Engine...");
    scriptEngine = std::make_unique<ScriptEngine>();
    // Only init if pointer is valid
    if (scriptEngine) {
      scriptEngine->Init(*registry, &camera);
      TraceLog(LOG_INFO, "SYSTEM: Script Engine Ready.");
    }
  } catch (...) {
    TraceLog(LOG_ERROR, "CRITICAL: Script Engine failed to initialize!");
  }

  editor = std::make_unique<Editor>(this);

#ifndef BD_SHIPPING
  isEditorMode = true;
#else
  isEditorMode = false;
#endif
}

Engine::~Engine() {
  SaveConfig();
#ifndef BD_SHIPPING
  rlImGuiShutdown();
#endif
  UnloadRenderTexture(viewportTarget);
  UnloadRenderTexture(gameTarget);
  CloseWindow();
}

void Engine::Exit() { isRunning = false; }

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
        if (key == "LastTheme")
          lastThemePath = value;
        if (key == "LastFont")
          lastFontPath = value;
        if (key == "ShowGrid")
          showGrid = (value == "true");
        if (key == "GridSize")
          gridSize = std::stoi(value);
        if (key == "ScreenWidth")
          targetWidth = std::stoi(value);
        if (key == "ScreenHeight")
          targetHeight = std::stoi(value);
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
    file << "ScreenWidth=" << screenWidth << "\n";
    file << "ScreenHeight=" << screenHeight << "\n";
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
  const int TOTAL_ENTITIES = 2500;
  const float WORLD_WIDTH = 1200.0f;
  const float WORLD_HEIGHT = 800.0f;

  Entity floor = registry->CreateEntity();
  registry->AddComponent(
      floor,
      TransformComponent{{WORLD_WIDTH / 2, WORLD_HEIGHT - 20}, {1, 1}, 0});
  registry->AddComponent(floor, VelocityComponent{{0, 0}});
  registry->AddComponent(floor,
                         RigidPhysicsComponent{1.0f, 0.2f, 0.0f, 1.0f, true});
  registry->AddComponent(
      floor, BoxColliderComponent{{WORLD_WIDTH, 40.0f}, {0, 0}, true});

  for (int i = 0; i < 10; i++) {
    Entity peg = registry->CreateEntity();
    float x = (WORLD_WIDTH / 10) * i + 50;
    float y = 300 + (i % 2 * 100);

    registry->AddComponent(peg, TransformComponent{{x, y}, {1, 1}, 0});
    registry->AddComponent(peg, VelocityComponent{{0, 0}});
    registry->AddComponent(peg,
                           RigidPhysicsComponent{1.0f, 0.5f, 0.0f, 1.0f, true});

    if (i % 2 == 0) {
      registry->AddComponent(peg, CircleColliderComponent{{0, 0}, 20.0f, true});
    } else {
      registry->AddComponent(
          peg, BoxColliderComponent{{60.0f, 20.0f}, {0, 0}, true});
    }
  }

  for (int i = 0; i < TOTAL_ENTITIES; i++) {
    Entity e = registry->CreateEntity();

    float rx = (float)GetRandomValue(100, (int)WORLD_WIDTH - 100);
    float ry = (float)GetRandomValue(-1000, -50);

    registry->AddComponent(e, TransformComponent{{rx, ry}, {1, 1}, 0});
    registry->AddComponent(
        e, VelocityComponent{{(float)GetRandomValue(-20, 20), 0}});

    float mass = (float)GetRandomValue(1, 5);
    float bounce = (float)GetRandomValue(2, 8) / 10.0f;
    registry->AddComponent(
        e, RigidPhysicsComponent{mass, bounce, 1.0f, 1.0f, false});

    if (GetRandomValue(0, 1) == 0) {
      float radius = (float)GetRandomValue(10, 25);
      registry->AddComponent(e, CircleColliderComponent{{0, 0}, radius, true});
    } else {
      float size = (float)GetRandomValue(20, 40);
      registry->AddComponent(e,
                             BoxColliderComponent{{size, size}, {0, 0}, true});
    }
  }
  editorAssets = AssetScanner::Scan("assets");
  ScanThemes();

  for (const auto &asset : editorAssets) {
    if (asset.isTexture) {
      assets.LoadTextureAsset(asset.name, asset.path);
    }
  }
  // TODO - Create a persistent scene system
  // SceneManager::LoadScene("assets/scenes/main.scene", *registry, this);
}

void Engine::Update() {
  double startTime = GetTime();

  // DYNAMIC RESOLUTION UPDATE
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
  if (IsKeyPressed(KEY_F1))
    isEditorMode = !isEditorMode;

  if (IsKeyPressed(KEY_TAB)) {
#ifndef BD_SHIPPING
    if (playState == Stopped) {
      // SceneManager::SaveScene("temp_play.bds", *registry);
      playState = Playing;
    } else {
      // SceneManager::LoadScene("temp_play.bds", *registry, this);
      playState = Stopped;
    }
#else
    isEditorMode = false;
    TraceLog(LOG_WARNING, "EDITOR: Editor mode is disabled in this build!");
#endif
  }

  if (IsKeyPressed(KEY_F11) ||
      (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER)))
    ToggleFullscreen();
  if (IsKeyPressed(KEY_F2))
    stats.Toggle();

  // V-Sync Toggle
  if (IsKeyPressed(KEY_F3)) {
    if (IsWindowState(FLAG_VSYNC_HINT)) {
      ClearWindowState(FLAG_VSYNC_HINT);
      SetTargetFPS(0);
    } else {
      SetWindowState(FLAG_VSYNC_HINT);
    }
  }

  // MAIN LOGIC
  if (!showSettings) {
#ifndef BD_SHIPPING
    // Editor UI logic always runs in dev
    editor->Update();

    if (isEditorMode) {
      bool isTyping = (activeControlId != 0);
      if (!isTyping && IsMouseOverViewport) {
        // Right-Click Pan
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
          Vector2 delta = GetMouseDelta();
          camera.target.x -= delta.x / camera.zoom;
          camera.target.y -= delta.y / camera.zoom;
        }

        // Zoom Logic (Centered on Mouse)
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
          Vector2 mouseWorldPos =
              GetScreenToWorld2D(GetMousePosition(), camera);
          camera.offset = GetMousePosition();
          camera.target = mouseWorldPos;
          camera.zoom += wheel * zoomSensitivity;
          camera.zoom = Clamp(camera.zoom, 0.1f, 5.0f);
        }
      }
    }
#endif

    // SIMULATION LOGIC (Always runs if Playing)
    if (playState == Playing) {
      InputSystem::Update(*registry);
      ControlSystem::Update(*registry);

      float dt = GetFrameTime();
      const float fixedDeltaTime = 0.024f;
      physicsTimeAccumulator += dt;

      AnimationSystem::Update(*registry, dt);

      while (physicsTimeAccumulator >= fixedDeltaTime) {
        physicsSystem.UpdatePhysics(fixedDeltaTime, *registry, physicsGrid);
        MovementSystem::Update(*registry, fixedDeltaTime);
        physicsTimeAccumulator -= fixedDeltaTime;
      }

      if (scriptEngine) {
        for (Entity i : registry->activeEntities) {
          if (registry->HasComponent(i, COMP_SCRIPT)) {
            auto &script = registry->scripts[i];
            for (const std::string &path : script.scriptPaths) {
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
}

void Engine::Render() {
  // 1. DRAW RAW GAME TO gameTarget
  BeginTextureMode(gameTarget);
  ClearBackground(currentTheme.background);
  BeginMode2D(camera);
  RenderSystem::Draw(*registry);
  DebugSystem::PhysicsDebug(*registry, camera);
  EndMode2D();
  EndTextureMode();

  // 2. DRAW SCENE VIEW (With Overlays) TO viewportTarget
  BeginTextureMode(viewportTarget);
  ClearBackground(currentTheme.background);

  // Draw the clean game view first
  Rectangle src = {0, 0, (float)gameTarget.texture.width,
                   (float)-gameTarget.texture.height};
  Vector2 pos = {0, 0};
  DrawTextureRec(gameTarget.texture, src, pos, WHITE);

  BeginMode2D(camera);
  if (showGrid) {
    EditorSystem::DrawGrid(gridSize, camera, screenWidth, screenHeight,
                           gridColor);
  }

  if (selectedEntity != -1 &&
      registry->HasComponent(selectedEntity, COMP_TRANSFORM)) {
    auto &t = registry->transforms[selectedEntity];
    if (registry->HasComponent(selectedEntity, COMP_SPRITE)) {
      auto &s = registry->sprites[selectedEntity];
      Rectangle outline = {t.position.x - 2, t.position.y - 2,
                           (s.texture.width * t.scale.x) + 4,
                           (s.texture.height * t.scale.y) + 4};
      DrawRectangleLinesEx(outline, 2.0f / camera.zoom, ORANGE);
    }
  }

  EndMode2D();
  EndTextureMode();

  // 2. DRAW UI AND EDITOR TO THE ACTUAL SCREEN
  BeginDrawing();
  ClearBackground(DARKGRAY);

#ifndef BD_SHIPPING
  if (isEditorMode) {
    // TO: Always run the ImGui lifecycle if we are in a non-shipping build.
    // This prevents the "Forgot to call Render()" crash when toggling modes.
    rlImGuiBegin();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(
        ImVec2((float)GetScreenWidth(), (float)GetScreenHeight()));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |=
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("BDEngine_MasterDockHost", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // Main DockSpace
    ImGuiID dockspace_id = ImGui::GetID("BDEngineDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    editor->Render();

    ImGui::End();

    rlImGuiEnd();
  }
#endif

  // 3. DRAW GAME VIEWPORT (If not handled by ImGui window)
  if (!isEditorMode) {
    DrawTexturePro(
        gameTarget.texture,
        Rectangle{0, 0, (float)gameTarget.texture.width,
                  (float)-gameTarget.texture.height},
        Rectangle{0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
        Vector2{0, 0}, 0, WHITE);

    UISystem::Draw(*registry);
    DebugSystem::Draw(*registry, stats, GetScreenWidth());
  }

  EndDrawing();
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

  TraceLog(LOG_INFO, "THEME_SYSTEM: Scan complete. Total themes found: %d",
           themeFiles.size());
  UnloadDirectoryFiles(files);
}

void Engine::ScanFonts() {
  fontFiles.clear();
  FilePathList files = LoadDirectoryFiles("assets/fonts");
  for (unsigned int i = 0; i < files.count; i++) {
    if (IsFileExtension(files.paths[i], ".ttf") ||
        IsFileExtension(files.paths[i], ".otf")) {
      fontFiles.push_back(files.paths[i]);
      TraceLog(LOG_INFO, "FONT_SYSTEM: Found font: %s", files.paths[i]);
    }
  }
  UnloadDirectoryFiles(files);
}

void Engine::LoadEngineFont(const std::string &path) {
  if (!FileExists(path.c_str())) {
    TraceLog(LOG_WARNING, "FONT_SYSTEM: Font file not found: %s", path.c_str());
    return;
  }

  if (engineFont.texture.id != 0)
    UnloadFont(engineFont);

  engineFont = LoadFontEx(path.c_str(), 24, 0, 250);

  SetTextureFilter(engineFont.texture, TEXTURE_FILTER_BILINEAR);

  GuiSetFont(engineFont);

  // ApplyTheme(lastThemePath);

  lastFontPath = path;

  TraceLog(LOG_INFO, "FONT_SYSTEM: Successfully loaded font: %s", path.c_str());
}

void Engine::ApplyTheme(const std::string &filePath) {
  TraceLog(LOG_INFO, "THEME_SYSTEM: Attempting to apply theme: %s",
           filePath.c_str());

  char *text = LoadFileText(filePath.c_str());
  if (text == NULL) {
    TraceLog(LOG_ERROR, "THEME_SYSTEM: Failed to load theme file text!");
    return;
  }

  std::string content(text);
  UnloadFileText(text);

  auto parseColor = [&](const std::string &key) -> Color {
    size_t pos = content.find(key + ": ");
    if (pos != std::string::npos) {
      int r, g, b, a;
      const char *start = content.c_str() + pos + key.length() + 2;
      if (sscanf(start, "%d, %d, %d, %d", &r, &g, &b, &a) == 4) {
        TraceLog(LOG_INFO, "THEME_SYSTEM: Parsed %s -> R:%d G:%d B:%d A:%d",
                 key.c_str(), r, g, b, a);
        return {(unsigned char)r, (unsigned char)g, (unsigned char)b,
                (unsigned char)a};
      }
    }
    TraceLog(LOG_WARNING, "THEME_SYSTEM: Could not parse key: %s", key.c_str());
    return MAGENTA;
  };

  currentTheme.background = parseColor("bg");
  currentTheme.panelBG = parseColor("panel");
  currentTheme.gridColor = parseColor("grid");
  currentTheme.accentColor = parseColor("accent");

  this->gridColor = currentTheme.gridColor;

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
  TraceLog(LOG_INFO, "THEME_SYSTEM: Theme '%s' applied and saved to config.",
           filePath.c_str());
}