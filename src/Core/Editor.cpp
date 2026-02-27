#include "Editor.h"
#include "ECS/Registry.h"
#include "ECS/UISystem.h"
#include "Engine.h"
#include "Managers/ResourceManager.h"
#include "Utils/AssetEntry.h"
#include "Utils/AssetManager.h"
#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <filesystem>

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
        Vector2 worldMouse =
            GetScreenToWorld2D(GetMousePosition(), owner->GetCamera());
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

      if (owner->selectedEntity != -1 && IsKeyPressed(KEY_DELETE)) {
        owner->registry->entityMasks[owner->selectedEntity].reset();
        owner->selectedEntity = -1;
      }
    }

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
      draggedAssetIndex = -1;

    // Rescan logic moved to top of Update() so it runs
    // even when ImGui has mouse focus (e.g. clicking in browser)
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

  DrawSceneView();
  DrawGameView();

  // 3. ASSET BROWSER / PALETTE WINDOW
  ImGui::Begin(currentMode == MODE_WORLD ? "Asset Browser" : "UI Palette");
  if (currentMode == MODE_WORLD) {
    EditorSystem::DrawAssetBrowser(owner->editorAssets, currentBrowserPath,
                                   draggedAssetIndex);
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
    Vector2 mPos = GetMousePosition();
    auto &asset = owner->editorAssets[draggedAssetIndex];
    if (asset.isTexture && asset.preview.id != 0) {
      DrawTextureEx(asset.preview, {mPos.x - 20, mPos.y - 20}, 0, 1.0f,
                    Fade(WHITE, 0.6f));
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

void DrawUIInspector(Entity e, Registry &reg, float xPos, float &currentY,
                     float panelWidth, Engine *engine) {
  auto &ui = reg.uiComponents[e];

  if (ImGui::CollapsingHeader("UI Component", ImGuiTreeNodeFlags_DefaultOpen)) {
    const char *types[] = {"PANEL", "BUTTON", "LABEL", "IMAGE"};
    int typeInt = (int)ui.type;
    if (ImGui::Combo("Type", &typeInt, types, 4)) {
      ui.type = (UIType)typeInt;
    }

    if (ui.type != UI_IMAGE) {
      char textBuf[128];
      strcpy(textBuf, ui.text.c_str());
      if (ImGui::InputText("Text", textBuf, 128)) {
        ui.text = textBuf;
      }
    }

    ImGui::DragFloat2("Offset", &ui.offset.x, 1.0f);
    ImGui::DragFloat2("Size", &ui.size.x, 1.0f);

    const char *anchors[] = {"Top Left", "Top Right", "Center", "Bottom Left"};
    int anchorInt = (int)ui.anchor;
    if (ImGui::Combo("Anchor", &anchorInt, anchors, 4)) {
      ui.anchor = (UIAnchor)anchorInt;
    }

    if (ui.type == UI_IMAGE || ui.type == UI_BUTTON || ui.type == UI_PANEL) {
      ImGui::ColorEdit4("Color", (float *)&ui.color,
                        ImGuiColorEditFlags_NoInputs |
                            ImGuiColorEditFlags_Uint8);
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
  ImGui::TextDisabled("ENTITY %i", e);
  ImGui::Separator();

  if (ImGui::BeginChild("InspectorContent")) {

    // TRANSFORM COMPONENT
    if (reg.HasComponent(e, COMP_TRANSFORM)) {
      if (ImGui::CollapsingHeader("Transform",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &t = reg.transforms[e];
        ImGui::DragFloat2("Position", &t.position.x, 1.0f);
        float deg = t.rotation;
        if (ImGui::DragFloat("Rotation", &deg, 0.5f)) {
          t.rotation = deg;
        }
        ImGui::DragFloat2("Scale", &t.scale.x, 0.01f, 0.001f, 100.0f);
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
        ImGui::ColorEdit4("Tint", (float *)&s.tint,
                          ImGuiColorEditFlags_NoInputs |
                              ImGuiColorEditFlags_Uint8);
        ImGui::Checkbox("Flip X", &s.flipX);
        ImGui::DragFloat2("Anchor", &s.anchor.x, 0.1f, 0.0f, 1.0f);
        ImGui::EndGroup();
      }
    }

    // VELOCITY
    if (reg.HasComponent(e, COMP_VELOCITY)) {
      if (ImGui::CollapsingHeader("Velocity", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &v = reg.velocities[e];
        ImGui::DragFloat2("Velocity", &v.speed.x, 0.5f);
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
          ImGui::BulletText("%s", GetFileName(sc.scriptPaths[i].c_str()));
          ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
          ImGui::PushID(i);
          if (ImGui::Button("X")) {
            sc.scriptPaths.erase(sc.scriptPaths.begin() + i);
            i--;
          }
          ImGui::PopID();
        }
      }
    }

    // RIGID PHYSICS
    if (reg.HasComponent(e, COMP_RIGIDPHYSICS)) {
      if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &rp = reg.rigidPhysicsComponents[e];
        ImGui::DragFloat("Mass", &rp.mass, 0.1f, 0.0f, 1000.0f);
        ImGui::DragFloat("Restitution", &rp.restitution, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Friction", &rp.friction, 0.01f, 0.0f, 1.0f);
        ImGui::Checkbox("Static", &rp.isStatic);
        ImGui::Checkbox("Gravity", &rp.affectedByGravity);
      }
    }

    // COLLIDERS
    if (reg.HasComponent(e, COMP_CIRCLECOLLIDER)) {
      if (ImGui::CollapsingHeader("Circle Collider",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &cc = reg.circleColliders[e];
        ImGui::DragFloat("Radius", &cc.radius, 1.0f, 0.1f, 1000.0f);
        ImGui::DragFloat2("Offset", &cc.offset.x, 0.5f);
        ImGui::Checkbox("Show Debug", &cc.debugDraw);
      }
    }

    // ANIMATION
    if (reg.HasComponent(e, COMP_SPRITE_ANIMATION)) {
      if (ImGui::CollapsingHeader("Animation",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &sa = reg.spriteAnimations[e];
        ImGui::DragInt("Frames", &sa.frameCount, 1, 1, 128);
        ImGui::DragInt("Rows", &sa.rowCount, 1, 1, 16);
        float fps = (sa.frameDuration > 0) ? (1.0f / sa.frameDuration) : 0;
        if (ImGui::DragFloat("FPS", &fps, 1.0f, 1.0f, 120.0f)) {
          sa.frameDuration = 1.0f / fps;
        }
        ImGui::Checkbox("Loop", &sa.loop);
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
                      std::string &currentPath, int &draggedAssetIndex) {
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
