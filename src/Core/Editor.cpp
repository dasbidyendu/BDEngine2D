#include "Editor.h"
#include "Engine.h"
#include "ECS/Registry.h"
#include "Managers/ResourceManager.h"
#include "Utils/AssetEntry.h"
#include "ECS/UISystem.h"
#include "raygui.h"
#include "raymath.h"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

void Editor::Update() {
    Vector2 mousePos = GetMousePosition();
    bool mouseInSidebar = (mousePos.x < 250);
    bool mouseInInspector = (mousePos.x > (GetScreenWidth() - 300));
    bool mouseInUI = mouseInSidebar || mouseInInspector;

    static bool isDraggingUI = false;

    if (currentMode == MODE_UI_EDITOR) {
        if (isDraggingUI && owner->selectedEntity != -1 && owner->registry->HasComponent(owner->selectedEntity, COMP_UI)) {
            if (IsMouseButtonUp(MOUSE_LEFT_BUTTON)) {
                isDraggingUI = false;
            }
            else {
                auto& ui = owner->registry->uiComponents[owner->selectedEntity];
                Vector2 delta = GetMouseDelta();
                ui.offset = Vector2Add(ui.offset, delta);
                return;
            }
        }

        if (!mouseInUI && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            bool uiClicked = false;
            for (int i = MAX_ENTITIES - 1; i >= 0; i--) {
                if (owner->registry->HasComponent(i, COMP_UI)) {
                    auto& ui = owner->registry->uiComponents[i];
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
            }
            else {
                return;
            }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && draggedAssetIndex != -1) {
                draggedAssetIndex = -1;
        }
    }
    else {
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && draggedAssetIndex != -1) {
            if (!mouseInSidebar && !mouseInInspector) {
                auto& asset = owner->editorAssets[draggedAssetIndex];
                if (asset.isTexture) {
                    owner->assets.LoadTextureAsset(asset.name, asset.path);

                    Texture2D tex = owner->assets.GetTexture(asset.name);

                    Vector2 worldMouse = GetScreenToWorld2D(mousePos, owner->GetCamera());
                    Vector2 snappedPos = { floor(worldMouse.x / 32) * 32, floor(worldMouse.y / 32) * 32 };

                    Entity newEntity = owner->registry->CreateEntity();
                    owner->registry->AddComponent(newEntity, TransformComponent{ snappedPos, {1.0f, 1.0f}, 0.0f });

                    owner->registry->AddComponent(newEntity, SpriteComponent{
                        asset.name,
                        tex,
                        WHITE,
                        {0.5f, 0.5f},
                        false
                        });
                }
                draggedAssetIndex = -1;
            }
            else if (mouseInSidebar) {
                draggedAssetIndex = -1;
            }
        }

        if (!mouseInUI && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && draggedAssetIndex == -1) {
            Vector2 worldMouse = GetScreenToWorld2D(GetMousePosition(), owner->GetCamera());
            owner->selectedEntity = -1;

            for (Entity i = 0; i < MAX_ENTITIES; i++) {
                if (owner->registry->HasComponent(i, COMP_TRANSFORM) && owner->registry->HasComponent(i, COMP_SPRITE)) {
                    auto& t = owner->registry->transforms[i];
                    auto& s = owner->registry->sprites[i];
                    Rectangle bounds = { t.position.x - (s.texture.width * t.scale.x * s.anchor.x),
                                         t.position.y - (s.texture.height * t.scale.y * s.anchor.y),
                                         (float)s.texture.width * t.scale.x, (float)s.texture.height * t.scale.y };

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

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) draggedAssetIndex = -1;

    static float refreshTimer = 0;
    refreshTimer += GetFrameTime();
    if (refreshTimer > 2.0f) {
        if (fs::exists(currentBrowserPath)) {
            owner->editorAssets = AssetScanner::Scan(currentBrowserPath);
        }
        refreshTimer = 0;
    }

    if (currentBrowserPath != lastPath) {
        for (auto& asset : owner->editorAssets) {
            if (asset.preview.id != 0) UnloadTexture(asset.preview);
        }
        owner->editorAssets = AssetScanner::Scan(currentBrowserPath);
        lastPath = currentBrowserPath;
    }
}

void Editor::Render() {
    DrawTopBar();

    if (currentMode == MODE_WORLD) {
        EditorSystem::DrawAssetBrowser(owner->editorAssets, currentBrowserPath, draggedAssetIndex);

        if (owner->selectedEntity != -1) {
            if (owner->registry->entityMasks[owner->selectedEntity].any()) {
                EditorSystem::DrawInspector(owner->selectedEntity, *(owner->registry), GetScreenWidth(), GetScreenHeight(), owner);
                if (owner->registry->HasComponent(owner->selectedEntity, COMP_UICANVAS)) {
                    if (GuiButton({ (float)GetScreenWidth() / 2 - 100, (float)GetScreenHeight() - 100, 200, 40 }, "OPEN UI EDITOR")) {
                        currentMode = MODE_UI_EDITOR;
                        activeCanvasId = owner->selectedEntity;
                        owner->selectedEntity = -1;
                    }
                }
            }
            else {
                owner->selectedEntity = -1;
            }
        }

        if (draggedAssetIndex != -1) {
            Vector2 mPos = GetMousePosition();
            auto& asset = owner->editorAssets[draggedAssetIndex];

            if (asset.isTexture && asset.preview.id != 0) {
                DrawTextureEx(asset.preview, { mPos.x - 20, mPos.y - 20 }, 0, 1.0f, Fade(WHITE, 0.6f));
            }
            else {
                Rectangle iconRect = { mPos.x - 15, mPos.y - 20, 30, 40 };
                DrawRectangleRec(iconRect, Fade(SKYBLUE, 0.8f));
                DrawRectangleLinesEx(iconRect, 2, WHITE);
                DrawText("LUA", mPos.x - 12, mPos.y - 10, 10, WHITE);
            }
        }
    }
    else if (currentMode == MODE_UI_EDITOR) {
        DrawUIPalette();
        if (owner->selectedEntity != -1 && owner->registry->HasComponent(owner->selectedEntity, COMP_UI)) {
            float inspectorX = (float)GetScreenWidth() - 300;
            float y = 50;
            DrawRectangle(inspectorX, 40, 300, GetScreenHeight() - 40, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

            EditorSystem::DrawUIInspector(owner->selectedEntity, *(owner->registry), inspectorX, y, 300, owner);

            auto& ui = owner->registry->uiComponents[owner->selectedEntity];
            Rectangle r = UISystem::GetRect(ui);
            DrawRectangleLinesEx(r, 2, YELLOW);
            DrawCircle((int)r.x, (int)r.y, 4, YELLOW);
        }

        DrawRectangleLines(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(GREEN, 0.3f));
        DrawText("CANVAS EDIT MODE", GetScreenWidth() / 2 - 50, 50, 20, GREEN);
    }

    if (currentMode == MODE_WORLD) {
        EditorSystem::DrawSettingsMenu(owner->showSettings, owner->settingsActiveTab, *owner->registry, owner);
    }
}

void Editor::DrawTopBar() {
    float sw = (float)GetScreenWidth();
    DrawRectangle(0, 0, (int)sw, 40, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
    DrawLine(0, 40, (int)sw, 40, DARKGRAY);

    static bool showAddDropdown = false;
    if (GuiButton({ 10, 5, 80, 30 }, "ADD")) {
        showAddDropdown = !showAddDropdown;
    }

    if (currentMode == MODE_UI_EDITOR) {
        if (GuiButton({ 100, 5, 120, 30 }, "<- Back to World")) {
            currentMode = MODE_WORLD;
            activeCanvasId = -1;
            owner->selectedEntity = -1;
        }
        GuiLabel({ 230, 5, 200, 30 }, TextFormat("EDITING CANVAS: ID %i", activeCanvasId));
    }

    if (showAddDropdown) {
        Vector2 mouse = GetMousePosition();
        Rectangle menuRect = { 10, 40, 200, 100 };
        DrawRectangleRec(menuRect, DARKGRAY);
        DrawRectangleLinesEx(menuRect, 1, WHITE);

        if (currentMode == MODE_WORLD) {
            if (GuiButton({ 15, 45, 190, 25 }, "New UI Canvas")) {
                Entity e = owner->registry->CreateEntity();
                owner->registry->AddComponent(e, UICanvasComponent{ "Main Menu", true });
                showAddDropdown = false;
            }
        }
        if (GuiButton({ 15, 75, 190, 25 }, "Empty Entity")) {
            Entity e = owner->registry->CreateEntity();
            owner->registry->AddComponent(e, TransformComponent{ {0,0}, {1,1}, 0 });
            showAddDropdown = false;
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !CheckCollisionPointRec(mouse, menuRect) && !CheckCollisionPointRec(mouse, { 10, 5, 80, 30 })) {
            showAddDropdown = false;
        }
    }
}

void Editor::DrawUIPalette() {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float panelWidth = 250;

    DrawRectangle(0, 40, panelWidth, sh - 40, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
    DrawLine(panelWidth, 40, panelWidth, sh, DARKGRAY);
    GuiLabel({ 10, 50, 200, 20 }, "WIDGET PALETTE");

    const char* widgets[] = { "Button", "Text", "Image", "Panel" };
    UIType types[] = { UI_BUTTON, UI_TEXT, UI_IMAGE, UI_PANEL };

    for (int i = 0; i < 4; i++) {
        Rectangle btnRect = { 15, 80 + (i * 40.0f), panelWidth - 30, 30 };
        if (GuiButton(btnRect, widgets[i])) {
            Entity e = owner->registry->CreateEntity();
            UIComponent ui;
            ui.type = types[i];
            ui.parentCanvas = activeCanvasId;
            ui.name = widgets[i];
            ui.anchor = ANCHOR_CENTER;
            ui.offset = { (float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2 };

            if (ui.type == UI_BUTTON) { ui.size = { 120, 40 }; ui.text = "Button"; ui.color = DARKGRAY; }
            if (ui.type == UI_TEXT) { ui.size = { 100, 20 }; ui.text = "Label"; ui.color = BLANK; }
            if (ui.type == UI_IMAGE) { ui.size = { 100, 100 }; ui.color = WHITE; }
            if (ui.type == UI_PANEL) { ui.size = { 300, 200 }; ui.color = GRAY; }

            owner->registry->AddComponent(e, ui);
            owner->selectedEntity = e;
        }
    }
}

namespace EditorSystem {

    void DrawUIInspector(Entity e, Registry& reg, float xPos, float& currentY, float panelWidth, Engine* engine) {
        auto& ui = reg.uiComponents[e];
        float startY = currentY;
        float boxHeight = (ui.type == UI_IMAGE) ? 320.0f : 280.0f;
        GuiGroupBox({ xPos + 5, currentY, panelWidth - 10, boxHeight }, "UI COMPONENT");

        int typeInt = (int)ui.type;
        if (GuiDropdownBox({ xPos + 15, currentY + 20, panelWidth - 30, 24 }, "PANEL;BUTTON;LABEL;IMAGE", &typeInt, (engine->activeControlId == 50))) {
            engine->activeControlId = (engine->activeControlId == 50) ? 0 : 50;
            ui.type = (UIType)typeInt;
        }
        currentY += 55;

        if (ui.type != UI_IMAGE) {
            static char textBuf[128];
            static Entity lastE = -1;
            if (lastE != e) { strcpy(textBuf, ui.text.c_str()); lastE = e; }
            GuiLabel({ xPos + 15, currentY, 60, 24 }, "Text");
            if (GuiTextBox({ xPos + 65, currentY, panelWidth - 85, 24 }, textBuf, 128, (engine->activeControlId == 51))) {
                engine->activeControlId = (engine->activeControlId == 51) ? 0 : 51;
                ui.text = textBuf;
            }
            currentY += 30;
        }

        int ox = (int)ui.offset.x; int oy = (int)ui.offset.y;
        GuiLabel({ xPos + 15, currentY, 60, 24 }, "Offset");
        if (GuiValueBox({ xPos + 65, currentY, 80, 24 }, "X", &ox, -2000, 2000, (engine->activeControlId == 52))) engine->activeControlId = (engine->activeControlId == 52) ? 0 : 52;
        if (GuiValueBox({ xPos + 155, currentY, 80, 24 }, "Y", &oy, -2000, 2000, (engine->activeControlId == 53))) engine->activeControlId = (engine->activeControlId == 53) ? 0 : 53;
        ui.offset = { (float)ox, (float)oy };
        currentY += 30;

        int sw = (int)ui.size.x; int sh = (int)ui.size.y;
        GuiLabel({ xPos + 15, currentY, 60, 24 }, "Size");
        if (GuiValueBox({ xPos + 65, currentY, 80, 24 }, "W", &sw, 1, 2000, (engine->activeControlId == 54))) engine->activeControlId = (engine->activeControlId == 54) ? 0 : 54;
        if (GuiValueBox({ xPos + 155, currentY, 80, 24 }, "H", &sh, 1, 2000, (engine->activeControlId == 55))) engine->activeControlId = (engine->activeControlId == 55) ? 0 : 55;
        ui.size = { (float)sw, (float)sh };
        currentY += 35;

        GuiLabel({ xPos + 15, currentY, 60, 24 }, "Anchor");
        for (int i = 0; i < 4; i++) {
            Rectangle b = { xPos + 65 + (i * 42), currentY, 40, 24 };
            if (GuiToggle(b, (i == 0 ? "TL" : i == 1 ? "TR" : i == 2 ? "C" : "BL"), ui.anchor == i)) ui.anchor = (UIAnchor)i;
        }
        currentY += 35;

        if (ui.type == UI_IMAGE) {
            Rectangle dropZone = { xPos + 15, currentY, panelWidth - 30, 40 };
            bool isHovered = CheckCollisionPointRec(GetMousePosition(), dropZone);
            DrawRectangleRec(dropZone, isHovered ? Fade(GOLD, 0.3f) : Fade(GRAY, 0.1f));
            GuiLabel(dropZone, ui.texturePath.empty() ? "  Drop Texture Here" : TextFormat("  %s", GetFileName(ui.texturePath.c_str())));
            if (isHovered && engine->editor->draggedAssetIndex != -1 && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                auto& asset = engine->editorAssets[engine->editor->draggedAssetIndex];
                if (asset.isTexture) {
                    ui.texturePath = asset.path;
                    ui.texture = engine->assets.GetTexture(asset.name);

                    engine->editor->draggedAssetIndex = -1;
                }
            }
            currentY += 45;
        }

        GuiLabel({ xPos + 15, currentY, 60, 24 }, "Tint");
        DrawRectangleRec({ xPos + 65, currentY, 40, 24 }, ui.color);
        if (GuiButton({ xPos + 110, currentY, 100, 24 }, "PICK COLOR")) {}

        currentY = startY + boxHeight + 10;
    }

    void DragFloat(const char* label, float* value, float speed, Rectangle bounds, int controlId, Engine* engine) {
        bool isPressed = CheckCollisionPointRec(GetMousePosition(), bounds) && IsMouseButtonDown(MOUSE_LEFT_BUTTON);

        // If clicking and moving, treat as a slider
        if (isPressed && engine->activeControlId == 0) {
            Vector2 delta = GetMouseDelta();
            *value += delta.x * speed;
        }

        // Standard RayGui ValueBox for manual entry
        int tempVal = (int)*value;
        if (GuiValueBox(bounds, label, &tempVal, -9999, 9999, engine->activeControlId == controlId)) {
            engine->activeControlId = (engine->activeControlId == controlId) ? 0 : controlId;
        }
        *value = (float)tempVal;
    }

    void DragFloat(const char* label, float* value, float speed, Rectangle bounds, int controlId, int min, int max, Engine* engine) {
        bool isPressed = CheckCollisionPointRec(GetMousePosition(), bounds) && IsMouseButtonDown(MOUSE_LEFT_BUTTON);

        if (isPressed && engine->activeControlId == 0) {
            Vector2 delta = GetMouseDelta();
            *value += delta.x * speed;
        }

        int tempVal = (int)*value;
        if (GuiValueBox(bounds, label, &tempVal, min, max, engine->activeControlId == controlId)) {
            engine->activeControlId = (engine->activeControlId == controlId) ? 0 : controlId;
        }
        *value = (float)tempVal;
    }
    void DragInt(const char* label, int* value, float speed, Rectangle bounds, int controlId, int min, int max, Engine* engine) {
        Vector2 mousePos = GetMousePosition();
        bool isHovered = CheckCollisionPointRec(mousePos, bounds);
        int typingId = controlId + 1000;

        bool isTyping = (engine->activeControlId == typingId);
        bool isDragging = (engine->activeControlId == controlId);

        static int typingBuffer = 0;

        if (engine->activeControlId != 0 && !isTyping && !isDragging) {
            GuiValueBox(bounds, label, value, min, max, false);
            return;
        }

        if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            typingBuffer = *value;
            engine->activeControlId = typingId;
        }

        if (isTyping && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Vector2 delta = GetMouseDelta();
            if (fabs(delta.x) > 2.0f) {
                engine->activeControlId = controlId;
            }
        }

        if (engine->activeControlId == controlId) {
            Vector2 delta = GetMouseDelta();
            *value += (int)(delta.x * speed);
            if (*value < min) *value = min;
            if (*value > max) *value = max;

            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) engine->activeControlId = 0;
        }

        if (GuiValueBox(bounds, label, isTyping ? &typingBuffer : value, INT_MIN, INT_MAX, isTyping)) {
            if (isTyping) {
                *value = Clamp(typingBuffer, min, max);
                engine->activeControlId = 0;
            }
        }

        if (isTyping && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !isHovered) {
            *value = Clamp(typingBuffer, min, max);
            engine->activeControlId = 0;
        }
    }
    void IntBox(const char* label, int* value, Rectangle bounds, int controlId, int min, int max, Engine* engine) {
        Vector2 mousePos = GetMousePosition();
        bool isHovered = CheckCollisionPointRec(mousePos, bounds);

        int typingId = controlId + 1000;
        bool isTyping = (engine->activeControlId == typingId);

        static int typingBuffer = 0;

        if (GuiValueBox(bounds, label, isTyping ? &typingBuffer : value, INT_MIN, INT_MAX, isTyping)) {
            if (!isTyping) {
                typingBuffer = *value;
                engine->activeControlId = typingId;
            }
            else {
                *value = (typingBuffer < min) ? min : (typingBuffer > max ? max : typingBuffer);
                engine->activeControlId = 0;
            }
        }

        if (isTyping) {
            int safeVal = typingBuffer;
            if (safeVal < min) safeVal = min;
            if (safeVal > max) safeVal = max;
            *value = safeVal;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !isHovered) {
                engine->activeControlId = 0;
            }
        }
    }

    void FloatBox(const char* label, float* value, Rectangle bounds, int controlId, float min, float max, Engine* engine) {
        Vector2 mousePos = GetMousePosition();
        bool isHovered = CheckCollisionPointRec(mousePos, bounds);

        int typingId = controlId + 2000;
        bool isTyping = (engine->activeControlId == typingId);

        static char textBuffer[64] = { 0 };

        if (!isTyping) {
            sprintf(textBuffer, "%.3f", *value);
        }

        if (GuiTextBox(bounds, textBuffer, 64, isTyping)) {
            if (!isTyping) {
                engine->activeControlId = typingId;
            }
            else {
                float parsed = atof(textBuffer);
                *value = (parsed < min) ? min : (parsed > max ? max : parsed);
                engine->activeControlId = 0;
            }
        }

        if (isTyping) {
            float liveParsed = atof(textBuffer);
            if (liveParsed >= min && liveParsed <= max) {
                *value = liveParsed;
            }

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !isHovered) {
                float finalParsed = atof(textBuffer);
                *value = (finalParsed < min) ? min : (finalParsed > max ? max : finalParsed);
                engine->activeControlId = 0;
            }
        }
    }

    void DrawInspector(Entity e, Registry& reg, int screenWidth, int screenHeight, Engine* engine) {

        if (e < 0 || e >= MAX_ENTITIES || reg.entityMasks[e].none()) return;
        static Vector2 scrollOffset = { 0, 0 };
        static float contentHeight = 0;
        float panelWidth = 300.0f;
        float xPos = (float)screenWidth - panelWidth;
        float padding = 10.0f;

        float ctrlH = 24.0f;
        Rectangle viewBounds = { xPos,0,panelWidth,(float)screenHeight };
        Rectangle contentArea = { 0,0,panelWidth - 15,contentHeight };

        Rectangle view = GuiScrollPanel(viewBounds, NULL, contentArea, &scrollOffset);

        BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);

        float currentY = view.y + scrollOffset.y + 10.0f;

        DrawRectangleRec({ xPos, 0, panelWidth, (float)screenHeight }, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawLineEx({ xPos, 0 }, { xPos, (float)screenHeight }, 2.0f, DARKGRAY);

        GuiLabel({ xPos + padding, currentY, panelWidth, 30 }, TextFormat("#141# INSPECTOR: ENTITY %i", e));
        currentY += 40;



        //TRANSFORM COMPONENT
        if (reg.HasComponent(e, COMP_TRANSFORM)) {
            auto& t = reg.transforms[e];
            GuiGroupBox({ xPos + 5, currentY, panelWidth - 10, 110 }, "TRANSFORM");

            float labelW = 40;
            float inputW = (panelWidth - 65) / 2;


            GuiLabel({ xPos + 15, currentY + 20, labelW, ctrlH }, "Pos");
            DragFloat("X", &t.position.x, 1.0f, { xPos + 60, currentY + 20, inputW, ctrlH }, 1, engine);
            DragFloat("Y", &t.position.y, 1.0f, { xPos + 65 + inputW, currentY + 20, inputW, ctrlH }, 2, engine);


            GuiLabel({ xPos + 15, currentY + 50, labelW, ctrlH }, "Rot");
            DragFloat(NULL, &t.rotation, 0.5f, { xPos + 60, currentY + 50, panelWidth - 75, ctrlH }, 3, engine);

            float displayScaleX = t.scale.x * 100.0f;
            float displayScaleY = t.scale.y * 100.0f;

            GuiLabel({ xPos + 15, currentY + 80, labelW, ctrlH }, "Scl %");
            DragFloat("X", &displayScaleX, 1.0f, { xPos + 60, currentY + 80, inputW, ctrlH }, 4, engine);
            DragFloat("Y", &displayScaleY, 1.0f, { xPos + 65 + inputW, currentY + 80, inputW, ctrlH }, 5, engine);

            t.scale.x = displayScaleX / 100.0f;
            t.scale.y = displayScaleY / 100.0f;

            currentY += 120;
        }

        if (reg.HasComponent(e, COMP_VELOCITY)) {
            auto& v = reg.velocities[e];
            GuiGroupBox({ xPos + 5, currentY, panelWidth - 10, 80 }, "VELOCITY");
            float labelW = 40;
            float inputW = (panelWidth - 65) / 2;
            GuiLabel({ xPos + 15, currentY + 20, labelW, ctrlH }, "Vel");
            DragFloat("X", &v.speed.x, 1.0f, { xPos + 60, currentY + 20, inputW, ctrlH }, 6, engine);
            DragFloat("Y", &v.speed.y, 1.0f, { xPos + 65 + inputW, currentY + 20, inputW, ctrlH }, 7, engine);
            currentY += 90;
        }

        if (reg.HasComponent(e, COMP_SPRITE_ANIMATION)) {
            auto& sa = reg.spriteAnimations[e];
            GuiGroupBox({ xPos + 5, currentY, panelWidth - 10, 150 }, "SPRITE ANIMATION");
            IntBox("Frame Count", &sa.frameCount, { xPos + 15, currentY + 20, panelWidth - 30, ctrlH }, 8, 1, 100, engine);
            IntBox("Row Count", &sa.rowCount, { xPos + 15, currentY + 50, panelWidth - 30, ctrlH }, 9, 1, 100, engine);
            int fpsDisplay = (sa.frameDuration > 0) ? (int)(1.0f / sa.frameDuration) : 0;
            int originalFps = fpsDisplay;
            IntBox("FPS", &fpsDisplay, { xPos + 15, currentY + 80, panelWidth - 30, ctrlH }, 10, 1, 120, engine);
            if (fpsDisplay != originalFps && fpsDisplay > 0) {
                sa.frameDuration = 1.0f / (float)fpsDisplay;
            }
            currentY += 160;
        }

        /*if (reg.HasComponent(e, COMP_RIGIDPHYSICS)) {
            auto& rp = reg.rigidPhysicsComponents[e];
            GuiGroupBox({ xPos + 5, currentY, panelWidth - 10, 120 }, "RIGID PHYSICS");
            FloatBox("Mass", &rp.mass, { xPos + 15, currentY + 20, panelWidth - 30, ctrlH }, 11, 1, 1000, engine);
        }*/
        bool hasScript = reg.HasComponent(e, COMP_SCRIPT);
        Rectangle scriptBox = { xPos + 5, currentY, panelWidth - 10, hasScript ? 100.0f : 40.0f };

        // Handle Drag & Drop Logic for Scripts
        bool isHoveringScript = CheckCollisionPointRec(GetMousePosition(), scriptBox);
        bool draggingLua = (engine->editor->draggedAssetIndex != -1 &&
            engine->editorAssets[engine->editor->draggedAssetIndex].path.find(".lua") != std::string::npos);

        if (hasScript) {
            auto& sc = reg.scripts[e];
            scriptBox.height = 40.0f + (std::max((int)sc.scriptPaths.size(), 1) * 30.0f);

            if (isHoveringScript && draggingLua) {
                DrawRectangleRec(scriptBox, Fade(GOLD, 0.3f));
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    std::string path = engine->editorAssets[engine->editor->draggedAssetIndex].path;
                    if (std::find(sc.scriptPaths.begin(), sc.scriptPaths.end(), path) == sc.scriptPaths.end()) {
                        sc.scriptPaths.push_back(path);
                    }
                    engine->editor->draggedAssetIndex = -1;
                }
            }

            GuiGroupBox(scriptBox, "SCRIPTS");
            float sY = currentY + 25;
            for (int i = 0; i < (int)sc.scriptPaths.size(); i++) {
                GuiLabel({ xPos + 15, sY, panelWidth - 70, 25 }, GetFileName(sc.scriptPaths[i].c_str()));
                if (GuiButton({ xPos + panelWidth - 40, sY, 25, 25 }, "#113#")) {
                    sc.scriptPaths.erase(sc.scriptPaths.begin() + i);
                }
                sY += 30;
            }
            currentY += scriptBox.height + 10;
        }
        else {
            if (GuiButton(scriptBox, "+ ADD SCRIPT")) {
                reg.AddComponent(e, ScriptComponent{ {}, false });
            }
            // Logic to drop a script onto an entity that DOESN'T have the component yet
            if (isHoveringScript && draggingLua && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                std::string path = engine->editorAssets[engine->editor->draggedAssetIndex].path;

                if (reg.HasComponent(e, COMP_SCRIPT)) {
                    auto& sc = reg.scripts[e];
                    if (std::find(sc.scriptPaths.begin(), sc.scriptPaths.end(), path) == sc.scriptPaths.end()) {
                        sc.scriptPaths.push_back(path);
                    }
                }
                else {
                    ScriptComponent newSc;
                    newSc.scriptPaths.push_back(path);
                    reg.AddComponent(e, newSc);
                }

                engine->editor->draggedAssetIndex = -1; // Explicitly consume the asset here
            }
            currentY += 50;
        }

        if (reg.HasComponent(e, COMP_SPRITE)) {
            DrawSpriteEditor(e, reg, xPos, currentY, panelWidth, engine);
        }

        static int inspectorDropdownActive = 0;
        static bool showAddDropdown = false;
        const char* compNames = "ADD_COMP;TRANSFORM;SPRITE;VELOCITY;INPUT;SCRIPT;SPRITE ANIMATION;PHYSICS;CIRCLE_COLLIDER;";

        if (GuiDropdownBox({ xPos + 5, currentY ,panelWidth - padding, 20.0f }, compNames, &inspectorDropdownActive, showAddDropdown)) {
            showAddDropdown = !showAddDropdown;
        }

        if (!showAddDropdown && inspectorDropdownActive > 0) {
            switch (inspectorDropdownActive) {
            case 1: {
                if (!reg.HasComponent(e, COMP_TRANSFORM))
                    reg.AddComponent(e, TransformComponent{ {0,0}, {1,1}, 0 });
                break;
            }
            case 2: {
                if (!reg.HasComponent(e, COMP_SPRITE))
                    reg.AddComponent(e, SpriteComponent{ "assets/textures/test.png", {0}, WHITE, {0.5f,0.5f}, false });
                break;
            }
            case 3: {
                if (!reg.HasComponent(e, COMP_VELOCITY))
                    reg.AddComponent(e, VelocityComponent{ {10,10} });
                break;
            }
            case 4: {
                if (!reg.HasComponent(e, COMP_INPUT))
                    reg.AddComponent(e, InputComponent{});
                break;
            }
            case 5: {
                if (!reg.HasComponent(e, COMP_SCRIPT))
                    reg.AddComponent(e, ScriptComponent{ {}, false });
                break;
            }
            case 6: {
                if (!reg.HasComponent(e, COMP_SPRITE_ANIMATION))
                    reg.AddComponent(e, SpriteAnimationComponent{ 8, 8,0,0, 0.1f, 0.0f, true });
                break;
            }
            case 7: {
                if (!reg.HasComponent(e, COMP_RIGIDPHYSICS))
                    reg.AddComponent(e, RigidPhysicsComponent{ 1.0f, true });
                break;
            }
            case 8: {
                if (!reg.HasComponent(e, COMP_CIRCLECOLLIDER))
                    reg.AddComponent(e, CircleColliderComponent{ 25.0f, {0,0}, true });
                break;
            }
            }
            inspectorDropdownActive = 0;
            currentY += 60;

            if (GuiButton({ xPos + 5, (float)screenHeight - 40, panelWidth - 10, 30 }, "#158# DESTROY ENTITY")) {
                reg.entityMasks[e].reset();
            }



            EndScissorMode();

            contentHeight = (currentY - (view.y + scrollOffset.y)) + 300.0f;
        }
    }
        void DrawSettingsMenu(bool& open, int& activeTab, Registry & reg, Engine * engine) {
            if (!open) return;
            float sw = (float)GetScreenWidth();
            float sh = (float)GetScreenHeight();
            DrawRectangleRec({ 0, 0, sw, sh }, Fade(BLACK, 0.85f));
            if (GuiWindowBox({ 50, 50, sw - 100, sh - 100 }, "GLOBAL ENGINE SETTINGS")) open = false;
            const char* tabs[] = { "THEME", "GRAPHICS", "INPUT", "EDITOR" };
            GuiTabBar({ 60, 85, sw - 120, 30 }, tabs, 4, &activeTab);
        }

        void DrawAssetBrowser(std::vector<AssetEntry>&allAssets, std::string & currentPath, int& draggedAssetIndex) {
            float currentSH = (float)GetScreenHeight();
            float panelWidth = 250.0f;
            DrawRectangle(0, 0, panelWidth, currentSH, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
            DrawLineEx({ panelWidth, 0 }, { panelWidth, currentSH }, 2.0f, Fade(BLACK, 0.5f));

            if (GuiButton({ 5, 5, 40, 30 }, "#101#")) {
                currentPath = fs::path(currentPath).parent_path().string();
                std::replace(currentPath.begin(), currentPath.end(), '\\', '/');
            }
            GuiLabel({ 50, 5, panelWidth - 60, 30 }, currentPath.c_str());

            static Vector2 scroll = { 0, 0 };
            Rectangle viewArea = { 0, 40, panelWidth, currentSH - 70 };
            Rectangle content = { 0, 0, panelWidth - 20, (float)allAssets.size() * 50.0f };
            Rectangle view = GuiScrollPanel(viewArea, NULL, content, &scroll);

            BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);
            for (int i = 0; i < (int)allAssets.size(); i++) {
                const auto& asset = allAssets[i];
                float itemY = view.y + scroll.y + (i * 50);
                Rectangle slot = { 5, itemY, panelWidth - 25, 45 };

                if (itemY + 45 > view.y && itemY < view.y + view.height) {
                    bool hovered = CheckCollisionPointRec(GetMousePosition(), slot);
                    DrawRectangleRec(slot, hovered ? Fade(GOLD, 0.2f) : Fade(GRAY, 0.1f));

                    if (asset.isTexture && asset.preview.id != 0) DrawTexture(asset.preview, (int)slot.x + 2, (int)slot.y + 2, WHITE);
                    else if (asset.isFolder) {
                        DrawRectangle((int)slot.x + 10, (int)slot.y + 12, 30, 20, GOLD);
                        DrawRectangle((int)slot.x + 10, (int)slot.y + 10, 15, 5, GOLD);
                    }

                    if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (asset.isFolder) currentPath = asset.path;
                        else draggedAssetIndex = i;
                    }
                    DrawText(asset.name.c_str(), (int)slot.x + 50, (int)slot.y + 15, 12, hovered ? WHITE : LIGHTGRAY);
                }
            }
            EndScissorMode();
        }

        void DrawGrid(int size, Camera2D camera, int screenWidth, int screenHeight, Color color) {
            Vector2 topLeft = GetScreenToWorld2D({ 0, 0 }, camera);
            Vector2 bottomRight = GetScreenToWorld2D({ (float)screenWidth, (float)screenHeight }, camera);
            float startX = floor(topLeft.x / size) * size;
            float startY = floor(topLeft.y / size) * size;
            float endX = ceil(bottomRight.x / size) * size;
            float endY = ceil(bottomRight.y / size) * size;
            for (float x = startX; x <= endX; x += size) DrawLineEx({ x, startY }, { x, endY }, 1.0f / camera.zoom, color);
            for (float y = startY; y <= endY; y += size) DrawLineEx({ startX, y }, { endX, y }, 1.0f / camera.zoom, color);
        }

        void DrawSpriteEditor(Entity e, Registry & reg, float xPos, float& currentY, float panelWidth, Engine * engine) {
            auto& s = reg.sprites[e];
            auto& t = reg.transforms[e];
            static bool colorPickerActive = false;
            float boxHeight = colorPickerActive ? 360.0f : 210.0f;
            GuiGroupBox({ xPos + 5, currentY, panelWidth - 10, boxHeight }, "SPRITE PROPERTIES");

            GuiLabel({ xPos + 15, currentY + 20, panelWidth - 30, 20 }, TextFormat("Res: %ix%i", s.texture.width, s.texture.height));
            int width = (int)(s.texture.width * t.scale.x);
            int height = (int)(s.texture.height * t.scale.y);

            // If user edits pixels directly
            if (GuiValueBox({ xPos + 75, currentY + 45, 80, 24 }, "W", &width, 1, 4096, engine->activeControlId == 10)) {
                t.scale.x = (float)width / s.texture.width;
            }
            if (GuiValueBox({ xPos + 165, currentY + 45, 80, 24 }, "H", &height, 1, 4096, engine->activeControlId == 11)) {
                t.scale.y = (float)height / s.texture.height;
            }

            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++) {
                    Rectangle b = { xPos + 75 + (col * 22), currentY + 80 + (row * 22), 20, 20 };
                    bool isActive = (abs(s.anchor.x - (col * 0.5f)) < 0.1f && abs(s.anchor.y - (row * 0.5f)) < 0.1f);
                    if (GuiButton(b, isActive ? "#111#" : "")) { s.anchor.x = col * 0.5f; s.anchor.y = row * 0.5f; }
                }
            }

            DrawRectangleRec({ xPos + 75, currentY + 150, 24, 24 }, s.tint);
            if (GuiButton({ xPos + 105, currentY + 150, panelWidth - 120, 24 }, colorPickerActive ? "Close Picker" : "Edit Color")) colorPickerActive = !colorPickerActive;
            if (colorPickerActive) s.tint = GuiColorPicker({ xPos + 75, currentY + 180, 150, 150 }, "Tint Color", s.tint);

            float footerY = currentY + (colorPickerActive ? 330 : 180);
            if (GuiButton({ xPos + 15, footerY, (panelWidth - 30) / 2, 25 }, s.flipX ? "Flipped X" : "Normal X")) s.flipX = !s.flipX;
            if (GuiButton({ xPos + 15 + (panelWidth - 30) / 2 + 5, footerY, (panelWidth - 30) / 2 - 5, 25 }, "Reset")) { s.tint = WHITE; s.flipX = false; }
            currentY += boxHeight + 10;
        }
    
}
