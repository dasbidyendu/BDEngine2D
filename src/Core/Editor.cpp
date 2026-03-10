#include "Editor.h"
#include "ECS/Registry.h"
#include "ECS/UISystem.h"
#include "Engine.h"
#include "Managers/ResourceManager.h"
#include "Utils/AssetEntry.h"
#include "Utils/AssetManager.h"
#include "Utils/Logger.h"
#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

void Editor::Update() {
  // Asset rescan must run even when ImGui has mouse focus,
  // otherwise navigating folders in the browser never refreshes.
  static float refreshTimer = 0;
  refreshTimer += GetFrameTime();
  if (refreshTimer > 2.0f) {
    if (fs::exists(currentBrowserPath)) {
      // Unload active preview textures before rescan to prevent leaks
      for (auto &asset : owner->editorAssets) {
        if (asset.preview.id != 0)
          UnloadTexture(asset.preview);
      }
      owner->editorAssets = AssetScanner::Scan(currentBrowserPath);
    }
    refreshTimer = 0;
  }

  if (currentBrowserPath != lastPath) {
    for (auto &asset : owner->editorAssets) {
      if (asset.preview.id != 0)
        UnloadTexture(asset.preview);
    }
    owner->editorAssets = AssetScanner::Scan(currentBrowserPath);
    lastPath = currentBrowserPath;
  }

  if (ImGui::GetIO().WantCaptureMouse)
    return;

  ImVec2 viewportPos = ImGui::GetMainViewport()->Pos;

  Vector2 mousePos = GetMousePosition();
  Vector2 relativeMousePos = {mousePos.x - viewportPos.x,
                              mousePos.y - viewportPos.y};
  Vector2 worldMousePos =
      GetScreenToWorld2D(relativeMousePos, owner->GetCamera());

  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureMouse)
    return;

  static bool isDraggingUI = false;

  if (currentMode == MODE_UI_EDITOR) {
    if (isDraggingUI && owner->selectedEntity != -1 &&
        owner->registry->HasComponent(owner->selectedEntity, COMP_UI)) {
      if (IsMouseButtonUp(MOUSE_LEFT_BUTTON)) {
        isDraggingUI = false;
      } else {
        auto &ui = owner->registry->uiComponents[owner->selectedEntity];
        Vector2 delta = GetMouseDelta();
        ui.offset = Vector2Add(ui.offset, delta);
        return;
      }
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      bool uiClicked = false;
      for (int i = MAX_ENTITIES - 1; i >= 0; i--) {
        if (owner->registry->HasComponent(i, COMP_UI)) {
          auto &ui = owner->registry->uiComponents[i];
          if (ui.parentCanvas == activeCanvasId) {
            Rectangle rect = UISystem::GetRect(ui);
            if (CheckCollisionPointRec(mousePos, rect)) {
              owner->selectedEntity = i;
              isDraggingUI = true;
              uiClicked = true;
              break;
            }
          }
        }
      }

      if (!uiClicked) {
        owner->selectedEntity = -1;
      } else {
        return;
      }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && draggedAssetIndex != -1) {
      draggedAssetIndex = -1;
    }
  } else {
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && draggedAssetIndex != -1) {
      if (true) { // Replaced legacy coordinate check
        auto &asset = owner->editorAssets[draggedAssetIndex];
        if (asset.isTexture) {
          owner->assets.LoadTextureAsset(asset.name, asset.path);

          Texture2D tex = owner->assets.GetTexture(asset.name);

          Vector2 worldMouse = GetScreenToWorld2D(mousePos, owner->GetCamera());
          Vector2 snappedPos = {floor(worldMouse.x / 32) * 32,
                                floor(worldMouse.y / 32) * 32};

          Entity newEntity = owner->registry->CreateEntity();
          TransformComponent t;
          t.position = snappedPos;
          t.scale = {1.0f, 1.0f};
          t.rotation = 0.0f;
          t.padding = 0.0f;
          owner->registry->AddComponent(newEntity, t);

          SpriteComponent sprite;
          sprite.texturePath = asset.name;
          sprite.texture = tex;
          sprite.tint = WHITE;
          sprite.anchor = {0.5f, 0.5f};
          sprite.flipX = false;
          owner->registry->AddComponent(newEntity, sprite);
        }
        draggedAssetIndex = -1;
        // Legacy sidebar check removed
      }

      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && draggedAssetIndex == -1) {
        Vector2 mouseAbs = GetMousePosition();

        // Check if mouse is within Scene View
        if (mouseAbs.x >= sceneViewPos.x &&
            mouseAbs.x <= sceneViewPos.x + sceneViewSize.x &&
            mouseAbs.y >= sceneViewPos.y &&
            mouseAbs.y <= sceneViewPos.y + sceneViewSize.y) {

          Vector2 relativeMouse = {mouseAbs.x - sceneViewPos.x,
                                   mouseAbs.y - sceneViewPos.y};
          Vector2 worldMouse =
              GetScreenToWorld2D(relativeMouse, owner->GetCamera());

          owner->selectedEntity = -1;

          for (Entity i : owner->registry->activeEntities) {
            if (owner->registry->HasComponent(i, COMP_TRANSFORM) &&
                owner->registry->HasComponent(i, COMP_SPRITE)) {
              auto &t = owner->registry->transforms[i];
              auto &s = owner->registry->sprites[i];
              Rectangle bounds = {
                  t.position.x - (s.texture.width * t.scale.x * s.anchor.x),
                  t.position.y - (s.texture.height * t.scale.y * s.anchor.y),
                  (float)s.texture.width * t.scale.x,
                  (float)s.texture.height * t.scale.y};

              if (CheckCollisionPointRec(worldMouse, bounds)) {
                owner->selectedEntity = i;
                break;
              }
            }
          }
        }
      }

      if (owner->selectedEntity != -1 && IsKeyPressed(KEY_DELETE)) {
        owner->registry->DestroyEntity(owner->selectedEntity);
        owner->selectedEntity = -1;
      }
    }

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
      draggedAssetIndex = -1;

    // Rescan logic moved to top of Update() so it runs
    // even when ImGui has mouse focus (e.g. clicking in browser)
  }
}
void Editor::OpenScriptEditor(const std::string &path) {
  // If already open, just mark it visible and focus it
  for (auto &tab : openScriptTabs) {
    if (tab.filePath == path) {
      tab.open = true;
      ImGui::SetWindowFocus((std::string(fs::path(path).filename().string()) +
                             "###Script_" + path)
                                .c_str());
      return;
    }
  }

  // Read file contents
  std::ifstream file(path);
  if (!file.is_open())
    return;

  std::stringstream ss;
  ss << file.rdbuf();
  file.close();

  ScriptEditorTab tab;
  tab.filePath = path;
  tab.content = ss.str();
  tab.dirty = false;
  tab.open = true;
  openScriptTabs.push_back(std::move(tab));
}

void Editor::DrawScriptEditors() {
  for (int i = 0; i < (int)openScriptTabs.size(); i++) {
    auto &tab = openScriptTabs[i];
    if (!tab.open) {
      openScriptTabs.erase(openScriptTabs.begin() + i);
      i--;
      continue;
    }

    std::string filename = fs::path(tab.filePath).filename().string();
    std::string title =
        (tab.dirty ? "* " : "") + filename + "###Script_" + tab.filePath;

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(title.c_str(), &tab.open, ImGuiWindowFlags_MenuBar)) {
      // Menu bar with Save / Reload
      if (ImGui::BeginMenuBar()) {
        if (ImGui::MenuItem("Save", "Ctrl+S")) {
          std::ofstream out(tab.filePath);
          if (out.is_open()) {
            out << tab.content;
            out.close();
            tab.dirty = false;
            // Invalidate script cache so changes hot-reload
            if (owner->scriptEngine) {
              owner->scriptEngine->scriptCache.erase(tab.filePath);
            }
          }
        }
        if (ImGui::MenuItem("Reload")) {
          std::ifstream in(tab.filePath);
          if (in.is_open()) {
            std::stringstream ss;
            ss << in.rdbuf();
            in.close();
            tab.content = ss.str();
            tab.dirty = false;
          }
        }
        ImGui::EndMenuBar();
      }

      // Ctrl+S shortcut
      if (ImGui::IsWindowFocused() && ImGui::GetIO().KeyCtrl &&
          ImGui::IsKeyPressed(ImGuiKey_S)) {
        std::ofstream out(tab.filePath);
        if (out.is_open()) {
          out << tab.content;
          out.close();
          tab.dirty = false;
          if (owner->scriptEngine) {
            owner->scriptEngine->scriptCache.erase(tab.filePath);
          }
        }
      }

      // Text editor
      ImVec2 avail = ImGui::GetContentRegionAvail();
      // Reserve enough buffer space
      tab.content.reserve(tab.content.size() + 1024);
      if (ImGui::InputTextMultiline(
              "##ScriptCode", &tab.content[0], tab.content.capacity(), avail,
              ImGuiInputTextFlags_AllowTabInput |
                  ImGuiInputTextFlags_CallbackResize,
              [](ImGuiInputTextCallbackData *data) -> int {
                if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                  std::string *str = (std::string *)data->UserData;
                  str->resize(data->BufTextLen);
                  data->Buf = &(*str)[0];
                }
                return 0;
              },
              &tab.content)) {
        tab.content.resize(strlen(tab.content.c_str()));
        tab.dirty = true;
      }
    }
    ImGui::End();
  }
}

void Editor::DrawSceneView() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  if (ImGui::Begin("Scene")) {
    ImVec2 size = ImGui::GetContentRegionAvail();

    // Dynamic Resolution Matching
    if (size.x != owner->viewportTarget.texture.width ||
        size.y != owner->viewportTarget.texture.height) {
      if (size.x > 0 && size.y > 0) {
        UnloadRenderTexture(owner->viewportTarget);
        owner->viewportTarget = LoadRenderTexture((int)size.x, (int)size.y);
      }
    }

    ImVec2 screenPos = ImGui::GetCursorScreenPos();
    sceneViewPos = {screenPos.x, screenPos.y};
    sceneViewSize = {size.x, size.y};

    rlImGuiImageRenderTexture(&owner->viewportTarget);

    // Drop Target for Scene View (Spawning Entities)
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload *payload =
              ImGui::AcceptDragDropPayload("ASSET_PATH")) {
        const char *path = (const char *)payload->Data;
        std::string sPath(path);
        std::string ext = fs::path(sPath).extension().string();

        if (ext == ".png" || ext == ".jpg" || ext == ".bmp") {
          Entity e = owner->registry->CreateEntity();
          Vector2 mouseWorld =
              GetScreenToWorld2D(GetMousePosition(), owner->GetCamera());

          TransformComponent t;
          t.position = mouseWorld;
          t.scale = {1.0f, 1.0f};
          t.rotation = 0.0f;
          t.padding = 0.0f;
          owner->registry->AddComponent(e, t);

          VelocityComponent v;
          v.speed = {0.0f, 0.0f};
          v.pad[0] = 0.0f;
          v.pad[1] = 0.0f;
          owner->registry->AddComponent(e, v);

          SpriteComponent sprite;
          sprite.texturePath = sPath;
          owner->assets.LoadTextureAsset(sPath, sPath);
          sprite.texture = owner->assets.GetTexture(sPath);
          owner->registry->AddComponent(e, sprite);

          owner->selectedEntity = e;
        }
      }
      ImGui::EndDragDropTarget();
      draggedAssetIndex = -1;
    }

    // Input handling focused on this window
    owner->IsMouseOverViewport = ImGui::IsWindowHovered();
  }
  ImGui::End();
  ImGui::PopStyleVar();
}

void Editor::DrawGameView() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  if (ImGui::Begin("Game")) {
    ImVec2 size = ImGui::GetContentRegionAvail();

    // Dynamic Resolution Matching for Game View
    if (size.x != owner->gameTarget.texture.width ||
        size.y != owner->gameTarget.texture.height) {
      if (size.x > 0 && size.y > 0) {
        UnloadRenderTexture(owner->gameTarget);
        owner->gameTarget = LoadRenderTexture((int)size.x, (int)size.y);
      }
    }

    rlImGuiImageRenderTexture(&owner->gameTarget);
  }
  ImGui::End();
  ImGui::PopStyleVar();
}

void Editor::DrawTransportBar() {
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  // Center it at the top
  ImGui::SetNextWindowPos(
      ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + 25),
      ImGuiCond_Always, ImVec2(0.5f, 0.0f));
  ImGui::SetNextWindowSize(ImVec2(120, 32));

  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.7f));

  if (ImGui::Begin("##TransportBar", nullptr, flags)) {
    float btnSize = 24;

    // Play Button
    bool isPlaying = (owner->playState == Engine::Playing);
    if (isPlaying)
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button("PLAY", ImVec2(btnSize, btnSize))) {
      if (owner->playState == Engine::Stopped) {
        // SceneManager::SaveScene("temp_play.bds", *owner->registry);
        owner->playState = Engine::Playing;
      } else if (owner->playState == Engine::Paused) {
        owner->playState = Engine::Playing;
      }
    }
    if (isPlaying)
      ImGui::PopStyleColor();

    ImGui::SameLine();

    // Pause Button
    bool isPaused = (owner->playState == Engine::Paused);
    if (isPaused)
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button("PAUSE", ImVec2(btnSize, btnSize))) {
      if (owner->playState == Engine::Playing) {
        owner->playState = Engine::Paused;
      }
    }
    if (isPaused)
      ImGui::PopStyleColor();

    ImGui::SameLine();

    // Stop Button
    if (ImGui::Button("STOP", ImVec2(btnSize, btnSize))) {
      if (owner->playState != Engine::Stopped) {
        // SceneManager::LoadScene("temp_play.bds", *owner->registry, owner);
        owner->playState = Engine::Stopped;
      }
    }
  }
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
}

void Editor::Render() {
  DrawTransportBar();
  DrawMenuBar(); // Handles its own BeginMenuBar/EndMenuBar

  if (showConsole) {
    DrawConsole();
  }

  // 1.5. HIERARCHY WINDOW
  ImGui::Begin("Hierarchy");
  for (Entity i : owner->registry->activeEntities) {
    if (owner->registry->entityMasks[i].none())
      continue;
    std::string entityName = "Entity " + std::to_string(i);
    if (owner->registry->HasComponent(i, COMP_NAME)) {
      entityName = owner->registry->names[i].name;
    }

    bool isSelected = (owner->selectedEntity == i);
    std::string label = entityName + "##" + std::to_string(i);
    if (ImGui::Selectable(label.c_str(), isSelected)) {
      owner->selectedEntity = i;
    }
  }
  ImGui::End();

  DrawSceneView();
  DrawGameView();
  DrawScriptEditors();

  // 3. ASSET BROWSER / PALETTE WINDOW
  ImGui::Begin(currentMode == MODE_WORLD ? "Asset Browser" : "UI Palette");
  if (currentMode == MODE_WORLD) {
    EditorSystem::DrawAssetBrowser(owner->editorAssets, currentBrowserPath,
                                   draggedAssetIndex, this);
  } else {
    DrawUIPalette();
  }
  ImGui::End();

  // 4. INSPECTOR WINDOW
  ImGui::Begin("Inspector");
  if (owner->selectedEntity != -1) {
    // Safety check: verify entity index is still valid in the registry
    if (owner->selectedEntity < owner->registry->entityMasks.size()) {

      if (currentMode == MODE_WORLD) {
        if (owner->registry->entityMasks[owner->selectedEntity].any()) {
          // Pass current screen dimensions for the scroll logic
          EditorSystem::DrawInspector(owner->selectedEntity, *(owner->registry),
                                      GetScreenWidth(), GetScreenHeight(),
                                      owner);

          if (owner->registry->HasComponent(owner->selectedEntity,
                                            COMP_UICANVAS)) {
            // Fix: Use winPos and contentRegion for the button position so it
            // stays inside the ImGui window
            ImVec2 winPos = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetContentRegionAvail();
            if (GuiButton(
                    {winPos.x + 10, winPos.y + size.y + 40, size.x - 20, 30},
                    "OPEN UI EDITOR")) {
              currentMode = MODE_UI_EDITOR;
              activeCanvasId = owner->selectedEntity;
              owner->selectedEntity = -1;
            }
          }
        }
      } else if (currentMode == MODE_UI_EDITOR &&
                 owner->registry->HasComponent(owner->selectedEntity,
                                               COMP_UI)) {
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetContentRegionAvail();
        EditorSystem::DrawUIInspector(owner->selectedEntity, *(owner->registry),
                                      winPos.x, winPos.y, size.x, owner);
      }
    }
  } else {
    ImGui::Text("Select an entity to inspect");
  }
  ImGui::End();

  // 5. SETTINGS / MODALS
  if (currentMode == MODE_WORLD) {
    EditorSystem::DrawSettingsMenu(
        owner->showSettings, owner->settingsActiveTab, *owner->registry, owner);
  }

  // 6. ASSET DRAG PREVIEW (Floating)
  if (draggedAssetIndex != -1) {
    if (draggedAssetIndex < (int)owner->editorAssets.size()) {
      Vector2 mPos = GetMousePosition();
      auto &asset = owner->editorAssets[draggedAssetIndex];
      if (asset.isTexture && asset.preview.id != 0) {
        DrawTextureEx(asset.preview, {mPos.x - 20, mPos.y - 20}, 0, 1.0f,
                      Fade(WHITE, 0.6f));
      }
    } else {
      draggedAssetIndex = -1; // Stale index, reset
    }
  }
}

void Editor::DrawMenuBar() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Save Scene", "Ctrl+S")) { /* Logic */
      }
      if (ImGui::MenuItem("Exit")) {
        owner->Exit();
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Console", nullptr, &showConsole);
      ImGui::EndMenu();
    }

    // Mode Toggles inside Menu Bar
    float spacing = ImGui::GetContentRegionAvail().x - 150;
    ImGui::SameLine(spacing);
    if (ImGui::Button(currentMode == MODE_WORLD ? "Switch to UI Editor"
                                                : "Switch to World")) {
      currentMode = (currentMode == MODE_WORLD) ? MODE_UI_EDITOR : MODE_WORLD;
    }

    ImGui::EndMenuBar();
  }
}

void Editor::DrawUIPalette() {
  float sw = (float)GetScreenWidth();
  float sh = (float)GetScreenHeight();
  float panelWidth = 250;

  DrawRectangle(0, 40, panelWidth, sh - 40,
                GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
  DrawLine(panelWidth, 40, panelWidth, sh, DARKGRAY);
  GuiLabel({10, 50, 200, 20}, "WIDGET PALETTE");

  const char *widgets[] = {"Button", "Text", "Image", "Panel"};
  UIType types[] = {UI_BUTTON, UI_TEXT, UI_IMAGE, UI_PANEL};

  for (int i = 0; i < 4; i++) {
    Rectangle btnRect = {15, 80 + (i * 40.0f), panelWidth - 30, 30};
    if (GuiButton(btnRect, widgets[i])) {
      Entity e = owner->registry->CreateEntity();
      UIComponent ui;
      ui.type = types[i];
      ui.parentCanvas = activeCanvasId;
      ui.name = widgets[i];
      ui.anchor = ANCHOR_CENTER;
      ui.offset = {(float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2};

      if (ui.type == UI_BUTTON) {
        ui.size = {120, 40};
        ui.text = "Button";
        ui.color = DARKGRAY;
      }
      if (ui.type == UI_TEXT) {
        ui.size = {100, 20};
        ui.text = "Label";
        ui.color = BLANK;
      }
      if (ui.type == UI_IMAGE) {
        ui.size = {100, 100};
        ui.color = WHITE;
      }
      if (ui.type == UI_PANEL) {
        ui.size = {300, 200};
        ui.color = GRAY;
      }

      owner->registry->AddComponent(e, ui);
      owner->selectedEntity = e;
    }
  }
}

namespace EditorSystem {
void DrawGrid(int gridSize, Camera2D camera, int screenWidth, int screenHeight,
              Color color) {
  if (gridSize <= 0)
    return;

  Vector2 topLeft = GetScreenToWorld2D({0, 0}, camera);
  Vector2 bottomRight =
      GetScreenToWorld2D({(float)screenWidth, (float)screenHeight}, camera);

  // Snap to grid
  float startX = floor(topLeft.x / gridSize) * gridSize;
  float startY = floor(topLeft.y / gridSize) * gridSize;
  float endX = ceil(bottomRight.x / gridSize) * gridSize;
  float endY = ceil(bottomRight.y / gridSize) * gridSize;

  for (float x = startX; x <= endX; x += gridSize) {
    DrawLineV({x, startY}, {x, endY}, color);
  }
  for (float y = startY; y <= endY; y += gridSize) {
    DrawLineV({startX, y}, {endX, y}, color);
  }
}

static bool BeginPropertyGrid(const char *id) {
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
  return ImGui::BeginTable(id, 2,
                           ImGuiTableFlags_BordersInnerV |
                               ImGuiTableFlags_SizingStretchProp |
                               ImGuiTableFlags_Resizable);
}

static void EndPropertyGrid() {
  ImGui::EndTable();
  ImGui::PopStyleVar();
}

static void PropertyLabel(const char *label) {
  ImGui::TableNextColumn();
  float curY = ImGui::GetCursorPosY();
  ImGui::SetCursorPosY(curY + 3.0f); // slight vertical centering
  ImGui::TextUnformatted(label);
  ImGui::TableNextColumn();
  ImGui::SetNextItemWidth(-FLT_MIN);
}

static void DrawVec2Control(const std::string &label, Vector2 &values,
                            float resetValue = 0.0f) {
  PropertyLabel(label.c_str());

  ImGui::PushID(label.c_str());

  float lineHeight =
      ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
  ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};
  float widthEach = (ImGui::CalcItemWidth() - buttonSize.x * 2.0f) / 2.0f;

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
  if (ImGui::Button("X", buttonSize))
    values.x = resetValue;
  ImGui::PopStyleColor(3);

  ImGui::SameLine();
  ImGui::PushItemWidth(widthEach);
  ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
  ImGui::PopItemWidth();
  ImGui::SameLine();

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
  if (ImGui::Button("Y", buttonSize))
    values.y = resetValue;
  ImGui::PopStyleColor(3);

  ImGui::SameLine();
  ImGui::PushItemWidth(widthEach);
  ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
  ImGui::PopItemWidth();

  ImGui::PopStyleVar();
  ImGui::PopID();
}

static void DrawColorControl(const std::string &label, Color &color) {
  PropertyLabel(label.c_str());
  float col[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f,
                  color.a / 255.0f};
  ImGui::PushID(label.c_str());
  if (ImGui::ColorEdit4("##Color", col,
                        ImGuiColorEditFlags_NoInputs |
                            ImGuiColorEditFlags_Uint8)) {
    color.r = (unsigned char)(col[0] * 255.0f);
    color.g = (unsigned char)(col[1] * 255.0f);
    color.b = (unsigned char)(col[2] * 255.0f);
    color.a = (unsigned char)(col[3] * 255.0f);
  }
  ImGui::PopID();
}

void DrawUIInspector(Entity e, Registry &reg, float xPos, float &currentY,
                     float panelWidth, Engine *engine) {
  auto &ui = reg.uiComponents[e];

  if (ImGui::CollapsingHeader("UI Component", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (BeginPropertyGrid("ui_prop_grid")) {
      PropertyLabel("Type");
      const char *types[] = {"PANEL", "BUTTON", "LABEL", "IMAGE"};
      int typeInt = (int)ui.type;
      if (ImGui::Combo("##Type", &typeInt, types, 4)) {
        ui.type = (UIType)typeInt;
      }

      if (ui.type != UI_IMAGE) {
        PropertyLabel("Text");
        char textBuf[128];
        strcpy(textBuf, ui.text.c_str());
        if (ImGui::InputText("##Text", textBuf, 128)) {
          ui.text = textBuf;
        }
      }

      DrawVec2Control("Offset", ui.offset);
      DrawVec2Control("Size", ui.size);

      PropertyLabel("Anchor");
      const char *anchors[] = {"Top Left", "Top Right", "Center",
                               "Bottom Left"};
      int anchorInt = (int)ui.anchor;
      if (ImGui::Combo("##Anchor", &anchorInt, anchors, 4)) {
        ui.anchor = (UIAnchor)anchorInt;
      }

      if (ui.type == UI_IMAGE || ui.type == UI_BUTTON || ui.type == UI_PANEL) {
        DrawColorControl("Color", ui.color);
      }
      EndPropertyGrid();
    }
  }
}

void DrawInspector(Entity e, Registry &reg, int screenWidth, int screenHeight,
                   Engine *engine) {

  if (e < 0 || e >= reg.entityMasks.size() || reg.entityMasks[e].none()) {
    ImGui::Text("No entity selected");
    return;
  }

  ImGui::PushID(e);

  // Header with Entity ID
  if (reg.HasComponent(e, COMP_NAME)) {
    auto &n = reg.names[e];
    char nameBuf[128];
    strcpy(nameBuf, n.name.c_str());
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##EntityName", nameBuf, 128)) {
      n.name = nameBuf;
    }
  } else {
    ImGui::TextDisabled("ENTITY %i", e);
  }
  ImGui::Separator();

  if (ImGui::BeginChild("InspectorContent")) {

    // TRANSFORM COMPONENT
    if (reg.HasComponent(e, COMP_TRANSFORM)) {
      if (ImGui::CollapsingHeader("Transform",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &t = reg.transforms[e];
        if (BeginPropertyGrid("transform_grid")) {
          DrawVec2Control("Position", t.position);

          PropertyLabel("Rotation");
          float deg = t.rotation;
          if (ImGui::DragFloat("##Rotation", &deg, 0.5f)) {
            t.rotation = deg;
          }

          DrawVec2Control("Scale", t.scale, 1.0f);
          EndPropertyGrid();
        }
      }
    }

    // SPRITE COMPONENT
    if (reg.HasComponent(e, COMP_SPRITE)) {
      if (ImGui::CollapsingHeader("Sprite", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &s = reg.sprites[e];

        // Drop Target for Texture
        ImGui::BeginGroup();
        if (s.texture.id != 0) {
          float aspect = (float)s.texture.width / (float)s.texture.height;
          float w = 100.0f;
          float h = 100.0f;
          if (aspect > 1.0f)
            h = 100.0f / aspect;
          else
            w = 100.0f * aspect;
          rlImGuiImageSize(&s.texture, (int)w, (int)h);
        } else {
          ImGui::Button("No Texture", ImVec2(64, 64));
        }

        if (ImGui::BeginDragDropTarget()) {
          if (const ImGuiPayload *payload =
                  ImGui::AcceptDragDropPayload("ASSET_PATH")) {
            const char *path = (const char *)payload->Data;
            std::string sPath(path);
            std::string ext = fs::path(sPath).extension().string();
            if (ext == ".png" || ext == ".jpg" || ext == ".bmp") {
              s.texturePath = sPath;
              engine->assets.LoadTextureAsset(sPath, sPath);
              s.texture = engine->assets.GetTexture(sPath);
            }
          }
          ImGui::EndDragDropTarget();
        }
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("Path: %s", GetFileName(s.texturePath.c_str()));

        if (BeginPropertyGrid("sprite_grid")) {
          DrawColorControl("Tint", s.tint);

          PropertyLabel("Flip X");
          ImGui::Checkbox("##FlipX", &s.flipX);

          DrawVec2Control("Anchor", s.anchor);
          EndPropertyGrid();
        }
        ImGui::EndGroup();
      }
    }

    // VELOCITY
    if (reg.HasComponent(e, COMP_VELOCITY)) {
      if (ImGui::CollapsingHeader("Velocity", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &v = reg.velocities[e];
        if (BeginPropertyGrid("velocity_grid")) {
          DrawVec2Control("Speed", v.speed);
          EndPropertyGrid();
        }
      }
    }

    // INPUT COMPONENT
    if (reg.HasComponent(e, COMP_INPUT)) {
      if (ImGui::CollapsingHeader("Input", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &in = reg.inputComponents[e];
        if (BeginPropertyGrid("input_grid")) {
          PropertyLabel("Up");
          ImGui::TextDisabled("%s", in.up ? "TRUE" : "FALSE");
          PropertyLabel("Down");
          ImGui::TextDisabled("%s", in.down ? "TRUE" : "FALSE");
          PropertyLabel("Left");
          ImGui::TextDisabled("%s", in.left ? "TRUE" : "FALSE");
          PropertyLabel("Right");
          ImGui::TextDisabled("%s", in.right ? "TRUE" : "FALSE");
          EndPropertyGrid();
        }
      }
    }

    // UICANVAS COMPONENT
    if (reg.HasComponent(e, COMP_UICANVAS)) {
      if (ImGui::CollapsingHeader("UI Canvas",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &canvas = reg.uiCanvases[e];
        if (BeginPropertyGrid("canvas_grid")) {
          PropertyLabel("Name");
          char nameBuf[128];
          strcpy(nameBuf, canvas.name.c_str());
          if (ImGui::InputText("##CanvasName", nameBuf, 128)) {
            canvas.name = nameBuf;
          }

          PropertyLabel("Active");
          ImGui::Checkbox("##CanvasActive", &canvas.isActive);
          EndPropertyGrid();
        }
      }
    }

    // SCRIPT COMPONENT
    if (reg.HasComponent(e, COMP_SCRIPT)) {
      if (ImGui::CollapsingHeader("Scripts", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &sc = reg.scripts[e];

        // Drop target for adding scripts
        ImGui::TextDisabled("(Drop .lua files here to add)");
        if (ImGui::BeginDragDropTarget()) {
          if (const ImGuiPayload *payload =
                  ImGui::AcceptDragDropPayload("ASSET_PATH")) {
            const char *path = (const char *)payload->Data;
            std::string sPath(path);
            if (fs::path(sPath).extension() == ".lua") {
              if (std::find(sc.scriptPaths.begin(), sc.scriptPaths.end(),
                            sPath) == sc.scriptPaths.end()) {
                sc.scriptPaths.push_back(sPath);
              }
            }
          }
          ImGui::EndDragDropTarget();
        }

        for (int i = 0; i < (int)sc.scriptPaths.size(); i++) {
          ImGui::PushID(i);
          ImGui::BulletText("%s", GetFileName(sc.scriptPaths[i].c_str()));

          // Edit button
          ImGui::SameLine(ImGui::GetContentRegionAvail().x - 45);
          if (ImGui::SmallButton("Edit")) {
            engine->editor->OpenScriptEditor(sc.scriptPaths[i]);
          }

          // Remove button
          ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
          if (ImGui::Button("X")) {
            sc.scriptPaths.erase(sc.scriptPaths.begin() + i);
            i--;
          }
          ImGui::PopID();
        }

        ImGui::Spacing();

        // Create New Script
        static char newScriptName[128] = "";
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 130);
        ImGui::InputText("##NewScriptName", newScriptName, 128);
        ImGui::SameLine();
        if (ImGui::Button("Create Script", ImVec2(120, 0))) {
          std::string name(newScriptName);
          if (!name.empty()) {
            std::string scriptPath = "assets/scripts/" + name + ".lua";
            // Create directories if needed
            fs::create_directories(fs::path(scriptPath).parent_path());
            // Write boilerplate
            std::ofstream out(scriptPath);
            if (out.is_open()) {
              out << "-- " << name << ".lua\n";
              out << "function OnUpdate(entity, dt)\n";
              out << "    -- Your code here\n";
              out << "end\n";
              out.close();

              // Add to entity
              if (std::find(sc.scriptPaths.begin(), sc.scriptPaths.end(),
                            scriptPath) == sc.scriptPaths.end()) {
                sc.scriptPaths.push_back(scriptPath);
              }

              // Open in editor
              engine->editor->OpenScriptEditor(scriptPath);
              newScriptName[0] = '\0';
            }
          }
        }
      }
    }

    // RIGID PHYSICS
    if (reg.HasComponent(e, COMP_RIGIDPHYSICS)) {
      if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &rp = reg.rigidPhysicsComponents[e];
        if (BeginPropertyGrid("physics_grid")) {
          PropertyLabel("Mass");
          ImGui::DragFloat("##Mass", &rp.mass, 0.1f, 0.0f, 1000.0f);
          PropertyLabel("Restitution");
          ImGui::DragFloat("##Restitution", &rp.restitution, 0.01f, 0.0f, 1.0f);
          PropertyLabel("Friction");
          ImGui::DragFloat("##Friction", &rp.friction, 0.01f, 0.0f, 1.0f);
          PropertyLabel("Static");
          ImGui::Checkbox("##Static", &rp.isStatic);
          PropertyLabel("Gravity");
          ImGui::Checkbox("##Gravity", &rp.affectedByGravity);
          EndPropertyGrid();
        }
      }
    }

    // CIRCLE COLLIDER
    if (reg.HasComponent(e, COMP_CIRCLECOLLIDER)) {
      if (ImGui::CollapsingHeader("Circle Collider",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &cc = reg.circleColliders[e];
        if (BeginPropertyGrid("circlecol_grid")) {
          PropertyLabel("Radius");
          ImGui::DragFloat("##Radius", &cc.radius, 1.0f, 0.1f, 1000.0f);
          DrawVec2Control("Offset", cc.offset);
          PropertyLabel("Show Debug");
          ImGui::Checkbox("##ShowDebugC", &cc.debugDraw);
          EndPropertyGrid();
        }
      }
    }

    // BOX COLLIDER
    if (reg.HasComponent(e, COMP_BOXCOLLIDER)) {
      if (ImGui::CollapsingHeader("Box Collider",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &bc = reg.boxColliders[e];
        if (BeginPropertyGrid("boxcol_grid")) {
          DrawVec2Control("Size", bc.size, 32.0f);
          DrawVec2Control("Offset", bc.offset);
          PropertyLabel("Show Debug");
          ImGui::Checkbox("##ShowDebugB", &bc.debugDraw);
          EndPropertyGrid();
        }
      }
    }

    // ANIMATION
    if (reg.HasComponent(e, COMP_SPRITE_ANIMATION)) {
      if (ImGui::CollapsingHeader("Animation",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &sa = reg.spriteAnimations[e];
        if (BeginPropertyGrid("anim_grid")) {
          PropertyLabel("Frames");
          ImGui::DragInt("##Frames", &sa.frameCount, 1, 1, 128);
          PropertyLabel("Rows");
          ImGui::DragInt("##Rows", &sa.rowCount, 1, 1, 16);

          PropertyLabel("FPS");
          float fps = (sa.frameDuration > 0) ? (1.0f / sa.frameDuration) : 0;
          if (ImGui::DragFloat("##FPS", &fps, 1.0f, 1.0f, 120.0f)) {
            sa.frameDuration = 1.0f / fps;
          }

          PropertyLabel("Loop");
          ImGui::Checkbox("##Loop", &sa.loop);
          EndPropertyGrid();
        }
      }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ADD COMPONENT BUTTON
    if (ImGui::Button("Add Component",
                      ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
      ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup")) {
      if (!reg.HasComponent(e, COMP_TRANSFORM) &&
          ImGui::MenuItem("Transform")) {
        TransformComponent t;
        t.position = {0, 0};
        t.scale = {1, 1};
        t.rotation = 0;
        t.padding = 0;
        reg.AddComponent(e, t);
      }
      if (!reg.HasComponent(e, COMP_SPRITE) && ImGui::MenuItem("Sprite")) {
        SpriteComponent s;
        s.texturePath = "assets/textures/test.png";
        s.texture = {0};
        s.tint = WHITE;
        s.anchor = {0.5f, 0.5f};
        s.flipX = false;
        reg.AddComponent(e, s);
      }
      if (!reg.HasComponent(e, COMP_VELOCITY) && ImGui::MenuItem("Velocity")) {
        VelocityComponent v;
        v.speed = {0, 0};
        v.pad[0] = 0;
        v.pad[1] = 0;
        reg.AddComponent(e, v);
      }
      if (!reg.HasComponent(e, COMP_SCRIPT) && ImGui::MenuItem("Script")) {
        ScriptComponent sc;
        sc.isInitialized = false;
        reg.AddComponent(e, sc);
      }
      if (!reg.HasComponent(e, COMP_RIGIDPHYSICS) &&
          ImGui::MenuItem("Physics")) {
        reg.AddComponent(e, RigidPhysicsComponent{});
      }
      if (!reg.HasComponent(e, COMP_CIRCLECOLLIDER) &&
          ImGui::MenuItem("Circle Collider")) {
        reg.AddComponent(e, CircleColliderComponent{});
      }
      if (!reg.HasComponent(e, COMP_BOXCOLLIDER) &&
          ImGui::MenuItem("Box Collider")) {
        reg.AddComponent(e, BoxColliderComponent{});
      }
      if (!reg.HasComponent(e, COMP_INPUT) && ImGui::MenuItem("Input")) {
        reg.AddComponent(e, InputComponent{});
      }
      if (!reg.HasComponent(e, COMP_UICANVAS) && ImGui::MenuItem("UI Canvas")) {
        reg.AddComponent(e, UICanvasComponent{});
      }
      if (!reg.HasComponent(e, COMP_SPRITE_ANIMATION) &&
          ImGui::MenuItem("Animation")) {
        SpriteAnimationComponent sa;
        sa.frameCount = 8;
        sa.rowCount = 1;
        sa.currentFrame = 0;
        sa.currentRow = 0;
        sa.frameDuration = 0.1f;
        sa.elapsedTime = 0.0f;
        sa.isPlaying = true;
        sa.loop = true;
        reg.AddComponent(e, sa);
      }

      ImGui::EndPopup();
    }
  }
  ImGui::EndChild();

  // DESTROY ENTITY BUTTON
  ImGui::Separator();
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
  if (ImGui::Button("DESTROY ENTITY",
                    ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
    reg.DestroyEntity(e);
    engine->selectedEntity = -1;
  }
  ImGui::PopStyleColor();

  ImGui::PopID();
}

void DrawSettingsMenu(bool &open, int &activeTab, Registry &reg,
                      Engine *engine) {
  if (!open)
    return;
  float sw = (float)GetScreenWidth();
  float sh = (float)GetScreenHeight();
  DrawRectangleRec({0, 0, sw, sh}, Fade(BLACK, 0.85f));
  if (GuiWindowBox({50, 50, sw - 100, sh - 100}, "GLOBAL ENGINE SETTINGS"))
    open = false;
  const char *tabs[] = {"THEME", "GRAPHICS", "INPUT", "EDITOR"};
  GuiTabBar({60, 85, sw - 120, 30}, tabs, 4, &activeTab);
}

void DrawAssetBrowser(std::vector<AssetEntry> &allAssets,
                      std::string &currentPath, int &draggedAssetIndex,
                      Editor *editor) {
  // Breadcrumbs / Back button
  if (ImGui::Button("Back")) {
    currentPath = fs::path(currentPath).parent_path().string();
    std::replace(currentPath.begin(), currentPath.end(), '\\', '/');
  }
  ImGui::SameLine();
  ImGui::Text("Path: %s", currentPath.c_str());

  ImGui::Separator();

  std::string newPath = currentPath;

  if (ImGui::BeginChild("AssetList")) {
    float iconSize = 40.0f;
    float padding = 10.0f;
    float cellSize = iconSize + padding;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1)
      columnCount = 1;

    if (ImGui::BeginTable("AssetGrid", columnCount)) {
      for (int i = 0; i < (int)allAssets.size(); i++) {
        auto &asset = allAssets[i];
        ImGui::TableNextColumn();
        ImGui::PushID(i);

        ImGui::BeginGroup();
        // Icon / Preview
        if (asset.isFolder) {
          ImGui::Button("FOLDER", ImVec2(iconSize, iconSize));
          if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            newPath = asset.path;
          }
        } else if (asset.isTexture && asset.preview.id != 0) {
          rlImGuiImage(&asset.preview);
        } else {
          ImGui::Button("FILE", ImVec2(iconSize, iconSize));
          // Double-click .lua files to open in script editor
          if (!asset.isFolder && ImGui::IsItemHovered() &&
              ImGui::IsMouseDoubleClicked(0)) {
            std::string ext = fs::path(asset.path).extension().string();
            if (ext == ".lua" && editor) {
              editor->OpenScriptEditor(asset.path);
            }
          }
        }

        // Label
        ImGui::TextWrapped("%s", asset.name.c_str());

        ImGui::EndGroup();

        // Navigate into folder on double-click
        if (asset.isFolder && ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(0)) {
          newPath = asset.path;
        }

        // Drag and Drop (files only)
        if (!asset.isFolder &&
            ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
          draggedAssetIndex = i;
          ImGui::SetDragDropPayload("ASSET_PATH", asset.path.c_str(),
                                    asset.path.size() + 1);
          ImGui::Text("Dragging %s", asset.name.c_str());
          ImGui::EndDragDropSource();
        }
        ImGui::PopID();
      }
      ImGui::EndTable();
    }
  }
  ImGui::EndChild();

  if (currentPath != newPath) {
    currentPath = newPath;
  }
}

} // namespace EditorSystem
void Editor::DrawConsole() {
  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Console", &showConsole, ImGuiWindowFlags_MenuBar)) {
    ImGui::End();
    return;
  }

  // --- Menu Bar (Filters & Clear) ---
  if (ImGui::BeginMenuBar()) {
    if (ImGui::Button("Clear")) {
      Logger::Clear();
    }
    ImGui::Separator();

    // Icon/Text for filters
    auto FilterButton = [](const char *label, bool *filter, ImVec4 color) {
      if (*filter)
        ImGui::PushStyleColor(ImGuiCol_Text, color);
      else
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

      if (ImGui::Button(label))
        *filter = !*filter;

      ImGui::PopStyleColor();
    };

    FilterButton("Info", &filterInfo, ImVec4(1, 1, 1, 1));
    ImGui::SameLine();
    FilterButton("Success", &filterSuccess, ImVec4(0, 1, 0, 1));
    ImGui::SameLine();
    FilterButton("Warn", &filterWarn, ImVec4(1, 1, 0, 1));
    ImGui::SameLine();
    FilterButton("Error", &filterError, ImVec4(1, 0, 0, 1));
    ImGui::SameLine();
    FilterButton("Raylib", &filterRaylib, ImVec4(0.5f, 0.5f, 1, 1));

    ImGui::EndMenuBar();
  }

  // --- Search & Options Bar ---
  ImGui::PushItemWidth(150);
  ImGui::InputText("Search", consoleSearch, IM_ARRAYSIZE(consoleSearch));
  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::Checkbox("Auto-scroll", &consoleAutoScroll);
  ImGui::Separator();

  // --- Log Display Area ---
  const float footer_height_to_reserve =
      ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
  ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve),
                    false, ImGuiWindowFlags_HorizontalScrollbar);

  if (ImGui::BeginPopupContextWindow()) {
    if (ImGui::Selectable("Clear"))
      Logger::Clear();
    ImGui::EndPopup();
  }

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(4, 1)); // Tighten spacing

  const auto &logs = Logger::GetLogs();
  for (const auto &log : logs) {
    // Apply filters
    if (!log.showInConsole)
      continue;

    bool show = false;
    if (log.level == LOG_LEVEL_INFO && filterInfo)
      show = true;
    if (log.level == LOG_LEVEL_WARNING && filterWarn)
      show = true;
    if (log.level == LOG_LEVEL_ERROR && filterError)
      show = true;
    if (log.level == LOG_LEVEL_SUCCESS && filterSuccess)
      show = true;
    if (log.level == LOG_LEVEL_RAYLIB && filterRaylib)
      show = true;

    if (!show)
      continue;

    // Search filter
    if (consoleSearch[0] != '\0' &&
        log.message.find(consoleSearch) == std::string::npos) {
      continue;
    }

    // Color code
    ImVec4 color = ImVec4(1, 1, 1, 1);
    const char *levelStr = "[INFO]";
    switch (log.level) {
    case LOG_LEVEL_WARNING:
      color = ImVec4(1, 1, 0.4f, 1);
      levelStr = "[WARN]";
      break;
    case LOG_LEVEL_ERROR:
      color = ImVec4(1, 0.4f, 0.4f, 1);
      levelStr = "[ERROR]";
      break;
    case LOG_LEVEL_SUCCESS:
      color = ImVec4(0.4f, 1, 0.4f, 1);
      levelStr = "[SUCCESS]";
      break;
    case LOG_LEVEL_RAYLIB:
      color = ImVec4(0.6f, 0.6f, 1, 1);
      levelStr = "[RAY]";
      break;
    }

    ImGui::TextDisabled("[%s]", log.timestamp.c_str());
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(levelStr);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextUnformatted(log.message.c_str());
  }

  if (consoleAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    ImGui::SetScrollHereY(1.0f);

  ImGui::PopStyleVar();
  ImGui::EndChild();

  ImGui::End();
}
