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

    // --- 1. UI ELEMENT SELECTION & DRAGGING (Highest Priority) ---
    static bool isDraggingUI = false;

    // If we are already dragging, keep dragging regardless of mouse position
    if (isDraggingUI && owner->selectedEntity != -1 && owner->registry->HasComponent(owner->selectedEntity, COMP_UI)) {
        if (IsMouseButtonUp(MOUSE_LEFT_BUTTON)) {
            isDraggingUI = false;
        }
        else {
            // Move UI
            auto& ui = owner->registry->uiComponents[owner->selectedEntity];
            Vector2 delta = GetMouseDelta();
            ui.offset = Vector2Add(ui.offset, delta);
            return; // Consume input
        }
    }

    // Try to select a UI Element
    if (!mouseInUI && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        bool uiClicked = false;

        // Iterate BACKWARDS to click the "top-most" UI element first
        for (int i = MAX_ENTITIES - 1; i >= 0; i--) {
            if (owner->registry->HasComponent(i, COMP_UI)) {
                auto& ui = owner->registry->uiComponents[i];
                Rectangle rect = UISystem::GetRect(ui);

                if (CheckCollisionPointRec(mousePos, rect)) {
                    owner->selectedEntity = i;
                    isDraggingUI = true;
                    uiClicked = true;
                    break;
                }
            }
        }

        // If we clicked UI, stop here. Don't select World entities.
        if (uiClicked) return;
    }

    // --- 2. ASSET DROPPING (Textures -> World Entities) ---
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && draggedAssetIndex != -1) {
        if (!mouseInUI) {
            auto& asset = owner->editorAssets[draggedAssetIndex];
            if (asset.isTexture) {
                Vector2 worldMouse = GetScreenToWorld2D(mousePos, owner->GetCamera());
                Vector2 snappedPos = { floor(worldMouse.x / 32) * 32, floor(worldMouse.y / 32) * 32 };

                Entity newEntity = owner->registry->CreateEntity();
                owner->registry->AddComponent(newEntity, TransformComponent{ snappedPos, {1.0f, 1.0f}, 0.0f });
                owner->registry->AddComponent(newEntity, SpriteComponent{ asset.name, owner->assets.GetTexture(asset.name), WHITE, {0.5f, 0.5f}, false });
            }
            draggedAssetIndex = -1;
        }
    }

    // Cancel drag if right click
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) draggedAssetIndex = -1;

    // --- 3. WORLD ENTITY SELECTION (Lowest Priority) ---
    if (!mouseInUI && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && draggedAssetIndex == -1 && !isDraggingUI) {
        Vector2 worldMouse = GetScreenToWorld2D(GetMousePosition(), owner->GetCamera());
        owner->selectedEntity = -1; // Deselect previous

        for (Entity i = 0; i < MAX_ENTITIES; i++) {
            if (owner->registry->HasComponent(i, COMP_TRANSFORM) && owner->registry->HasComponent(i, COMP_SPRITE)) {
                auto& t = owner->registry->transforms[i];
                auto& s = owner->registry->sprites[i];
                // Simple bounds check (improved slightly)
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

    // Deletion
    if (owner->selectedEntity != -1 && IsKeyPressed(KEY_DELETE)) {
        owner->registry->entityMasks[owner->selectedEntity].reset();
        owner->selectedEntity = -1;
    }

    // Asset Browser Refresh Logic
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
    EditorSystem::DrawAssetBrowser(owner->editorAssets, currentBrowserPath, draggedAssetIndex);

    if (owner->selectedEntity != -1) {
        if (owner->registry->entityMasks[owner->selectedEntity].any()) {
            EditorSystem::DrawInspector(owner->selectedEntity, *(owner->registry), GetScreenWidth(), GetScreenHeight(), owner);
        }
        else {
            owner->selectedEntity = -1;
        }
    }

    if (owner->selectedEntity != -1 && owner->registry->HasComponent(owner->selectedEntity, COMP_UI)) {
        auto& ui = owner->registry->uiComponents[owner->selectedEntity];
        Rectangle r = UISystem::GetRect(ui);

        // Draw outer glow/outline for selection
        DrawRectangleLinesEx(r, 2, YELLOW);
        DrawCircle((int)r.x, (int)r.y, 4, YELLOW); // Show origin point

        // Draw Anchor info text
        DrawText("Selected UI", (int)r.x, (int)r.y - 20, 10, YELLOW);
    }

    if (draggedAssetIndex != -1) {
        Vector2 mPos = GetMousePosition();
        auto& asset = owner->editorAssets[draggedAssetIndex];

        if (asset.isTexture && asset.preview.id != 0) {
            // Draw texture ghost
            DrawTextureEx(asset.preview, { mPos.x - 20, mPos.y - 20 }, 0, 1.0f, Fade(WHITE, 0.6f));
        }
        else {
            // Draw Script/File ghost icon
            Rectangle iconRect = { mPos.x - 15, mPos.y - 20, 30, 40 };
            DrawRectangleRec(iconRect, Fade(SKYBLUE, 0.8f));
            DrawRectangleLinesEx(iconRect, 2, WHITE);
            DrawText("LUA", mPos.x - 12, mPos.y - 10, 10, WHITE);
        }
    }

    EditorSystem::DrawSettingsMenu(owner->showSettings, owner->settingsActiveTab, *owner->registry, owner);
}

namespace EditorSystem {

    void DrawUIInspector(Entity e, Registry& reg, float xPos, float& currentY, float panelWidth) {
        auto& ui = reg.uiComponents[e];

        GuiGroupBox({ xPos + 5, currentY, panelWidth - 10, 200 }, "UI COMPONENT");

        // 1. Text Content
        GuiLabel({ xPos + 15, currentY + 20, 60, 24 }, "Text");

        // Note: Raygui GuiTextBox requires a char buffer. 
        // We copy string to buffer, edit, then copy back.
        static char textBuffer[128] = { 0 };
        static int lastEntity = -1;
        if (lastEntity != (int)e) { strcpy(textBuffer, ui.text.c_str()); lastEntity = e; }

        if (GuiTextBox({ xPos + 60, currentY + 20, panelWidth - 80, 24 }, textBuffer, 128, true)) {
            ui.text = std::string(textBuffer);
        }

        // 2. Size
        GuiLabel({ xPos + 15, currentY + 50, 60, 24 }, "Size");
        int w = (int)ui.size.x; int h = (int)ui.size.y;
        if (GuiValueBox({ xPos + 60, currentY + 50, 80, 24 }, "W", &w, 0, 2000, true)) ui.size.x = (float)w;
        if (GuiValueBox({ xPos + 160, currentY + 50, 80, 24 }, "H", &h, 0, 2000, true)) ui.size.y = (float)h;

        // 3. Anchor Buttons
        GuiLabel({ xPos + 15, currentY + 85, 60, 24 }, "Anchor");
        const char* anchors[] = { "TL", "TR", "C", "BL" };
        int activeAnchor = (int)ui.anchor;
        if (GuiToggleGroup({ xPos + 60, currentY + 85, 40, 24 }, "TL;TR;C;BL", activeAnchor)) {
            ui.anchor = (UIAnchor)activeAnchor;
        }

        // 4. Color
        GuiLabel({ xPos + 15, currentY + 120, 60, 24 }, "Color");
        DrawRectangleRec({ xPos + 60, currentY + 120, 40, 24 }, ui.color);
        if (GuiButton({ xPos + 110, currentY + 120, 100, 24 }, "Set Red")) ui.color = RED;
        // (We can connect the full color picker here later)

        currentY += 210;
    }

    void DrawInspector(Entity e, Registry& reg, int screenWidth, int screenHeight, Engine* engine) {
        if (e < 0 || e >= MAX_ENTITIES || reg.entityMasks[e].none()) return;

        //DYNAMIC LAYOUT CONFIG
        float panelWidth = 300.0f;
        float xPos = (float)screenWidth - panelWidth;
        float padding = 10.0f;
        float currentY = 10.0f;
        float controlHeight = 24.0f;


        // Background
        DrawRectangleRec({ xPos, 0, panelWidth, (float)screenHeight }, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawLineEx({ xPos, 0 }, { xPos, (float)screenHeight }, 2.0f, DARKGRAY);

        GuiLabel({ xPos + padding, currentY, panelWidth, 30 }, TextFormat("#141# INSPECTOR: ENTITY %i", e));
        currentY += 40;

        // TRANSFORM COMPONENT
        if (reg.HasComponent(e, COMP_TRANSFORM)) {
            auto& t = reg.transforms[e];
            GuiGroupBox({ xPos + 5, currentY, panelWidth - 10, 120 }, "TRANSFORM");

            float labelWidth = 45;
            float inputWidth = (panelWidth - 35 - labelWidth) / 2;
            float fullInputWidth = panelWidth - 30 - labelWidth;

            // Temporary variables for Raygui integer logic
            int posX = (int)t.position.x;
            int posY = (int)t.position.y;
            int rot = (int)t.rotation;
            int scl = (int)(t.scale.x * 100.0f);

            GuiLabel({ xPos + 15, currentY + 20, labelWidth, controlHeight }, "Pos");

            // X Input (ID: 1)
            if (GuiValueBox({ xPos + 15 + labelWidth, currentY + 20, inputWidth, controlHeight }, "X", &posX, -9999, 9999, (engine->activeControlId == 1))) {
                engine->activeControlId = (engine->activeControlId == 1) ? 0 : 1;
            }
            // Y Input (ID: 2)
            if (GuiValueBox({ xPos + 15 + labelWidth + inputWidth + 5, currentY + 20, inputWidth, controlHeight }, "Y", &posY, -9999, 9999, (engine->activeControlId == 2))) {
                engine->activeControlId = (engine->activeControlId == 2) ? 0 : 2;
            }

            // Rotation Input (ID: 3)
            GuiLabel({ xPos + 15, currentY + 50, labelWidth, controlHeight }, "Rot");
            if (GuiValueBox({ xPos + 15 + labelWidth, currentY + 50, fullInputWidth, controlHeight }, NULL, &rot, 0, 360, (engine->activeControlId == 3))) {
                engine->activeControlId = (engine->activeControlId == 3) ? 0 : 3;
            }

            // Scale Input (ID: 4)
            GuiLabel({ xPos + 15, currentY + 80, labelWidth, controlHeight }, "Scl %");
            if (GuiValueBox({ xPos + 15 + labelWidth, currentY + 80, fullInputWidth, controlHeight }, NULL, &scl, 1, 1000, (engine->activeControlId == 4))) {
                engine->activeControlId = (engine->activeControlId == 4) ? 0 : 4;
            }

            // Apply values back to the component
            t.position = { (float)posX, (float)posY };
            t.rotation = (float)rot;
            t.scale = { (float)scl / 100.0f, (float)scl / 100.0f };

            currentY += 130;
        }

        // SPRITE COMPONENT 
        if (reg.HasComponent(e, COMP_SPRITE)) {
            
            DrawSpriteEditor(e, reg, xPos, currentY, panelWidth, engine);
        }

        // SCRIPT COMPONENT 
        if (reg.HasComponent(e, COMP_SCRIPT)) {
            auto& sc = reg.scripts[e];
            float scriptEntryHeight = 30.0f;
            float boxHeight = 40.0f + (std::max((int)sc.scriptPaths.size(), 1) * scriptEntryHeight);
            Rectangle scriptBox = { xPos + 5, currentY, panelWidth - 10, boxHeight };

            bool isHovered = CheckCollisionPointRec(GetMousePosition(), scriptBox);
            bool isDraggingScript = false;

            // Asset dragging check
            if (engine->editor->draggedAssetIndex != -1) {
                if (engine->editorAssets[engine->editor->draggedAssetIndex].path.find(".lua") != std::string::npos)
                    isDraggingScript = true;
            }

            DrawRectangleRec(scriptBox, (isHovered && isDraggingScript) ? Fade(GOLD, 0.3f) : Fade(GRAY, 0.1f));
            GuiGroupBox(scriptBox, "SCRIPT COMPONENT (Multi)");

            // Drop Logic
            if (isHovered && isDraggingScript && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                std::string path = engine->editorAssets[engine->editor->draggedAssetIndex].path;
                if (std::find(sc.scriptPaths.begin(), sc.scriptPaths.end(), path) == sc.scriptPaths.end()) {
                    sc.scriptPaths.push_back(path);
                    engine->scriptEngine->scriptCache.erase(path);
                }
                engine->editor->draggedAssetIndex = -1;
            }

            // List Scripts
            float currentScriptY = currentY + 25;
            for (int i = 0; i < (int)sc.scriptPaths.size(); ) {
                Rectangle itemRect = { xPos + 10, currentScriptY, panelWidth - 60, 25 };
                GuiLabel(itemRect, TextFormat("#%i: %s", i + 1, GetFileName(sc.scriptPaths[i].c_str())));

                if (GuiButton({ xPos + panelWidth - 35, currentScriptY, 25, 25 }, "#113#")) {
                    sc.scriptPaths.erase(sc.scriptPaths.begin() + i);
                }
                else {
                    currentScriptY += scriptEntryHeight;
                    i++;
                }
            }

            if (sc.scriptPaths.empty()) {
                GuiLabel({ xPos + 15, currentY + 25, panelWidth - 30, 25 }, "EMPTY (DRAG SCRIPTS HERE)");
            }

            currentY += boxHeight + 10;
        }
        else {
            // Option to add a script component if the entity doesn't have one
            if (GuiButton({ xPos + 5, currentY, panelWidth - 10, 30 }, "+ ADD SCRIPT COMPONENT")) {
                reg.AddComponent(e, ScriptComponent{ {}, false });
            }
            currentY += 40;
        }

        if (reg.HasComponent(e, COMP_UI)) {
            DrawUIInspector(e, reg, xPos, currentY, panelWidth);
        }

        // GLOBAL DELETE BUTTON
        if (GuiButton({ xPos + 5, (float)screenHeight - 40, panelWidth - 10, 30 }, "#158# DELETE ENTITY")) {
            reg.entityMasks[e].reset();
        }
    }
    void DrawSettingsMenu(bool& open, int& activeTab, Registry& reg, Engine* engine) {
        if (!open) return;
        float sw = (float)GetScreenWidth();
        float sh = (float)GetScreenHeight();
        DrawRectangleRec({ 0, 0, sw, sh }, Fade(BLACK, 0.85f));

        if (GuiWindowBox({ 50, 50, sw - 100, sh - 100 }, "GLOBAL ENGINE SETTINGS")) open = false;

        const char* tabs[] = { "THEME", "GRAPHICS", "INPUT", "EDITOR" };
        GuiTabBar({ 60, 85, sw - 120, 30 }, tabs, 4, &activeTab);
    }

    void DrawAssetBrowser(std::vector<AssetEntry>& allAssets, std::string& currentPath, int& draggedAssetIndex) {
        float currentSW = (float)GetScreenWidth();
        float currentSH = (float)GetScreenHeight();
        float panelWidth = 250.0f;
        float footerHeight = 30.0f;
        float headerHeight = 40.0f;

        DrawRectangle(0, 0, panelWidth, currentSH, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawLineEx({ panelWidth, 0 }, { panelWidth, currentSH }, 2.0f, Fade(BLACK, 0.5f));

        // Address Bar / Back Button
        if (GuiButton({ 5, 5, 40, 30 }, "#101#")) {
            currentPath = fs::path(currentPath).parent_path().string();
            std::replace(currentPath.begin(), currentPath.end(), '\\', '/');
        }
        GuiLabel({ 50, 5, panelWidth - 60, 30 }, currentPath.c_str());

        // Scroll Logic
        static Vector2 scroll = { 0, 0 };
        Rectangle viewArea = { 0, headerHeight, panelWidth, currentSH - headerHeight - footerHeight };
        Rectangle content = { 0, 0, panelWidth - 20, (float)allAssets.size() * 50.0f };
        Rectangle view = GuiScrollPanel(viewArea, NULL, content, &scroll);

        BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);
        for (int i = 0; i < (int)allAssets.size(); i++) {
            const auto& asset = allAssets[i];
            float itemY = view.y + scroll.y + (i * 50);
            Rectangle slot = { 5, itemY, panelWidth - 25, 45 };

            // Optimization: Only draw if visible in the scroll window
            if (itemY + 45 > view.y && itemY < view.y + view.height) {
                bool hovered = CheckCollisionPointRec(GetMousePosition(), slot);

                // Highlight slot if hovered or being dragged
                DrawRectangleRec(slot, hovered ? Fade(GOLD, 0.2f) : Fade(GRAY, 0.1f));

                if (asset.isTexture && asset.preview.id != 0) {
                    DrawTexture(asset.preview, (int)slot.x + 2, (int)slot.y + 2, WHITE);
                }
                else if (asset.isFolder) {
                    DrawRectangle((int)slot.x + 10, (int)slot.y + 12, 30, 20, GOLD);
                    DrawRectangle((int)slot.x + 10, (int)slot.y + 10, 15, 5, GOLD);
                }

                if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (asset.isFolder) {
                        currentPath = asset.path;
                    }
                    else {
                        draggedAssetIndex = i;
                    }
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

    void EditorSystem::DrawSpriteEditor(Entity e, Registry& reg, float xPos, float& currentY, float panelWidth, Engine* engine) {
        auto& s = reg.sprites[e];
        auto& t = reg.transforms[e];
        float padding = 10.0f;
        float controlHeight = 24.0f;

        // We increase the GroupBox height to accommodate the color section
        static bool colorPickerActive = false;
        float boxHeight = colorPickerActive ? 360.0f : 210.0f;
        GuiGroupBox({ xPos + 5, currentY, panelWidth - 10, boxHeight }, "SPRITE PROPERTIES");

        // Texture Info & Size 
        GuiLabel({ xPos + 15, currentY + 20, panelWidth - 30, 20 }, TextFormat("Res: %ix%i", s.texture.width, s.texture.height));

        int width = (int)(s.texture.width * t.scale.x);
        int height = (int)(s.texture.height * t.scale.y);
        GuiLabel({ xPos + 15, currentY + 45, 60, controlHeight }, "Size");
        if (GuiValueBox({ xPos + 75, currentY + 45, 80, controlHeight }, "W", &width, 1, 4096, (engine->activeControlId == 10))) engine->activeControlId = (engine->activeControlId == 10) ? 0 : 10;
        if (GuiValueBox({ xPos + 165, currentY + 45, 80, controlHeight }, "H", &height, 1, 4096, (engine->activeControlId == 11))) engine->activeControlId = (engine->activeControlId == 11) ? 0 : 11;
        t.scale.x = (float)width / s.texture.width;
        t.scale.y = (float)height / s.texture.height;

        // Anchor Selector
        GuiLabel({ xPos + 15, currentY + 80, 60, controlHeight }, "Anchor");
        float anchorBoxSize = 20.0f;
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                Rectangle b = { xPos + 75 + (col * 22), currentY + 80 + (row * 22), anchorBoxSize, anchorBoxSize };
                bool isActive = (abs(s.anchor.x - (col * 0.5f)) < 0.1f && abs(s.anchor.y - (row * 0.5f)) < 0.1f);
                if (GuiButton(b, isActive ? "#111#" : "")) {
                    s.anchor.x = col * 0.5f;
                    s.anchor.y = row * 0.5f;
                }
            }
        }

        // TINT COLOR PICKER
        GuiLabel({ xPos + 15, currentY + 150, 60, controlHeight }, "Tint");

        // Draw a small preview rectangle of the current color
        DrawRectangleRec({ xPos + 75, currentY + 150, 24, 24 }, s.tint);
        DrawRectangleLinesEx({ xPos + 75, currentY + 150, 24, 24 }, 1, LIGHTGRAY);

        if (GuiButton({ xPos + 105, currentY + 150, panelWidth - 120, 24 }, colorPickerActive ? "Close Picker" : "Edit Color")) {
            colorPickerActive = !colorPickerActive;
        }

        if (colorPickerActive) {
            s.tint = GuiColorPicker({ xPos + 75, currentY + 180, 150, 150 }, "Tint Color", s.tint);
        }

        // --- 4. Flip Toggle (Adjusted Y position) ---
        float footerY = currentY + (colorPickerActive ? 330 : 180);
        if (GuiButton({ xPos + 15, footerY, (panelWidth - 30) / 2, 25 }, s.flipX ? "Flipped X" : "Normal X")) s.flipX = !s.flipX;
        if (GuiButton({ xPos + 15 + (panelWidth - 30) / 2 + 5, footerY, (panelWidth - 30) / 2 - 5, 25 }, "Reset")) {
            s.tint = WHITE;
            s.flipX = false;
        }

        currentY += boxHeight + 10;
    }
}