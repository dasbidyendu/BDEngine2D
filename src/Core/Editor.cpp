#include "Editor.h"
#include "Engine.h"
#include "ECS/Registry.h"
#include "Managers/ResourceManager.h"
#include "Utils/AssetEntry.h"
#include "raygui.h"
#include "raymath.h"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

void Editor::Update() {
    Vector2 mousePos = GetMousePosition();
    bool mouseInSidebar = (mousePos.x < 250);
    bool mouseInInspector = (mousePos.x > (GetScreenWidth() - 250));
    bool mouseInUI = mouseInSidebar || mouseInInspector;

    // 1. Handle World Dropping (Textures -> Entities)
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && draggedAssetIndex != -1) {
        if (!mouseInUI) {
            auto& asset = owner->editorAssets[draggedAssetIndex];
            if (asset.isTexture) {
                Vector2 worldMouse = GetScreenToWorld2D(mousePos, owner->GetCamera());
                Vector2 snappedPos = { floor(worldMouse.x / 32) * 32, floor(worldMouse.y / 32) * 32 };

                Entity newEntity = owner->registry->CreateEntity();
                owner->registry->AddComponent(newEntity, TransformComponent{ snappedPos, {1,1}, 0.0f });
                owner->registry->AddComponent(newEntity, SpriteComponent{ owner->assets.GetTexture(asset.name), WHITE });
            }
            draggedAssetIndex = -1;
        }
        // If mouseInUI is true, we DON'T reset here. 
        // We let the Render/Inspector logic handle the release.
    }

    // 2. Safety Reset (Only if released outside UI or Right-Clicked)
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) draggedAssetIndex = -1;
    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !mouseInUI) draggedAssetIndex = -1;

    // 3. Entity Selection
    if (!mouseInUI && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && draggedAssetIndex == -1) {
        Vector2 worldMouse = GetScreenToWorld2D(GetMousePosition(), owner->GetCamera());
        owner->selectedEntity = -1;

        for (Entity i = 0; i < MAX_ENTITIES; i++) {
            if (owner->registry->HasComponent(i, COMP_TRANSFORM) && owner->registry->HasComponent(i, COMP_SPRITE)) {
                auto& t = owner->registry->transforms[i];
                auto& s = owner->registry->sprites[i];

                Rectangle bounds = {
                    t.position.x, t.position.y,
                    (float)s.texture.width * t.scale.x,
                    (float)s.texture.height * t.scale.y
                };

                if (CheckCollisionPointRec(worldMouse, bounds)) {
                    owner->selectedEntity = i;
                    break;
                }
            }
        }
    }

    // 4. Deletion
    if (owner->selectedEntity != -1 && IsKeyPressed(KEY_DELETE)) {
        owner->registry->entityMasks[owner->selectedEntity].reset();
        owner->selectedEntity = -1;
    }

    // 5. Asset Browser Refresh Logic
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
    void DrawInspector(Entity e, Registry& reg, int screenWidth, int screenHeight, Engine* engine) {
        if (e < 0 || e >= MAX_ENTITIES || reg.entityMasks[e].none()) return;

        float width = 250;
        float x = (float)screenWidth - width;
        float padding = 10;
        float y = 40;

        DrawRectangle(x, 0, width, (float)screenHeight, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawLine(x, 0, x, screenHeight, DARKGRAY);
        GuiLabel({ x + padding, 10, width, 30 }, TextFormat("INSPECTOR (Entity %i)", e));

        // --- TRANSFORM COMPONENT ---
        if (reg.HasComponent(e, COMP_TRANSFORM)) {
            auto& t = reg.transforms[e];
            GuiGroupBox({ x + 5, y, width - 10, 110 }, "TRANSFORM");
            t.position.x = GuiSliderBar({ x + 40, y + 20, width - 60, 24 }, "X", TextFormat("%.2f", t.position.x), t.position.x, -2000, 2000);
            t.position.y = GuiSliderBar({ x + 40, y + 50, width - 60, 24 }, "Y", TextFormat("%.2f", t.position.y), t.position.y, -2000, 2000);
            t.scale.x = GuiSliderBar({ x + 50, y + 80, width - 70, 24 }, "Scl", TextFormat("%.2f", t.scale.x), t.scale.x, 0.1f, 10.0f);
            t.scale.y = t.scale.x;
            y += 130;
        }

        // --- SPRITE COMPONENT ---
        if (reg.HasComponent(e, COMP_SPRITE)) {
            auto& s = reg.sprites[e];
            GuiGroupBox({ x + 5, y, width - 10, 60 }, "SPRITE");
            if (GuiButton({ x + padding, y + 20, width - 30, 24 }, "Reset Tint")) s.tint = WHITE;
            y += 80;
        }

        // --- SCRIPT COMPONENT ---
        if (reg.HasComponent(e, COMP_SCRIPT)) {
            auto& sc = reg.scripts[e];
            float scriptEntryHeight = 30.0f;
            float boxHeight = 40.0f + (std::max((int)sc.scriptPaths.size(), 1) * scriptEntryHeight);
            Rectangle scriptBox = { x + 5, y, width - 10, boxHeight };

            bool isHovered = CheckCollisionPointRec(GetMousePosition(), scriptBox);
            bool isDraggingScript = false;

            if (engine->editor->draggedAssetIndex != -1) {
                if (engine->editorAssets[engine->editor->draggedAssetIndex].path.find(".lua") != std::string::npos)
                    isDraggingScript = true;
            }

            // Visual feedback for drag and drop
            DrawRectangleRec(scriptBox, (isHovered && isDraggingScript) ? Fade(GOLD, 0.3f) : Fade(GRAY, 0.1f));
            GuiGroupBox(scriptBox, "SCRIPT COMPONENT (Multi)");

            // 1. Drop Logic (The missing piece)
            if (isHovered && isDraggingScript && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                std::string path = engine->editorAssets[engine->editor->draggedAssetIndex].path;
                if (std::find(sc.scriptPaths.begin(), sc.scriptPaths.end(), path) == sc.scriptPaths.end()) {
                    sc.scriptPaths.push_back(path);
                    engine->scriptEngine->scriptCache.erase(path); // Refresh cache
                }
                engine->editor->draggedAssetIndex = -1;
            }

            // 2. Script List & Deletion Logic
            float currentScriptY = y + 25;
            for (int i = 0; i < (int)sc.scriptPaths.size(); ) {
                Rectangle itemRect = { x + 10, currentScriptY, width - 50, 25 };
                GuiLabel(itemRect, TextFormat("#%i: %s", i + 1, GetFileName(sc.scriptPaths[i].c_str())));

                if (GuiButton({ x + width - 35, currentScriptY, 25, 25 }, "#113#")) {
                    sc.scriptPaths.erase(sc.scriptPaths.begin() + i);
                    // Don't increment i, next item slides into this index
                }
                else {
                    currentScriptY += scriptEntryHeight;
                    i++;
                }
            }

            if (sc.scriptPaths.empty()) {
                GuiLabel({ x + 15, y + 25, width - 30, 25 }, "EMPTY (DRAG SCRIPTS HERE)");
            }

            y += boxHeight + 10;
        }
        else {
            if (GuiButton({ x + 5, y, width - 10, 30 }, "+ ADD SCRIPT COMPONENT")) {
                reg.AddComponent(e, ScriptComponent{ {}, false });
            }
            y += 40;
        }

        // --- GLOBAL DELETE BUTTON ---
        if (GuiButton({ x + 5, (float)screenHeight - 40, width - 10, 30 }, "DELETE ENTITY")) {
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
        float currentSH = (float)GetScreenHeight();
        float width = 250;
        float footerHeight = 30;
        float headerHeight = 40;

        DrawRectangle(0, 0, width, currentSH, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawLine(width, 0, width, currentSH, Fade(BLACK, 0.5f));

        // Address Bar / Back Button
        if (GuiButton({ 5, 5, 40, 30 }, "#101#")) { // Raygui back icon
            currentPath = fs::path(currentPath).parent_path().string();
            std::replace(currentPath.begin(), currentPath.end(), '\\', '/');
            // Trigger a re-scan here in your main logic
        }
        GuiLabel({ 50, 5, width - 60, 30 }, currentPath.c_str());

        static Vector2 scroll = { 0, 0 };
        Rectangle viewArea = { 0, headerHeight, width, currentSH - headerHeight - footerHeight };
        Rectangle content = { 0, 0, width - 20, (float)allAssets.size() * 50 };
        Rectangle view = GuiScrollPanel(viewArea, NULL, content, &scroll);

        BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);
        for (int i = 0; i < (int)allAssets.size(); i++) {
            const auto& asset = allAssets[i];
            float itemY = view.y + scroll.y + (i * 50);
            Rectangle slot = { 5, itemY, width - 25, 45 };

            if (itemY + 45 > view.y && itemY < view.y + view.height) {
                bool hovered = CheckCollisionPointRec(GetMousePosition(), slot);
                DrawRectangleRec(slot, hovered ? Fade(GOLD, 0.2f) : Fade(GRAY, 0.1f));

                // DRAW PREVIEW
                if (asset.isTexture && asset.preview.id != 0) {
                    DrawTexture(asset.preview, (int)slot.x + 2, (int)slot.y + 2, WHITE);
                }
                else if (asset.isFolder) {
                    DrawRectangle((int)slot.x + 5, (int)slot.y + 10, 30, 25, GOLD); // Placeholder Folder Icon
                }

                if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (asset.isFolder) {
                        currentPath = asset.path;
                        // Trigger a re-scan here main logic
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
}