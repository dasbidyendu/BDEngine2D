#pragma once
#include "raylib.h"
#include "raygui.h"
#include "Registry.h"
#include "Utils/AssetManager.h"


class Engine;

struct EngineStats {
    bool visible = false;
    float totalTime = 0.0f;
    uint64_t frameCount = 0;

    void Toggle() { visible = !visible; }
};

namespace MovementSystem {
    inline void Update(Registry& reg, float dt) {
        for (Entity i = 0; i < MAX_ENTITIES; i++) {
            if (reg.HasComponent(i, COMP_TRANSFORM) && reg.HasComponent(i, COMP_VELOCITY)) {
                reg.transforms[i].position.x += reg.velocities[i].speed.x * dt;
                reg.transforms[i].position.y += reg.velocities[i].speed.y * dt;
            }
        }
    }
}
namespace RenderSystem {
    inline void Draw(Registry& reg) {
        for (Entity i = 0; i < MAX_ENTITIES; i++) {
            if (reg.HasComponent(i, COMP_TRANSFORM) && reg.HasComponent(i, COMP_SPRITE)) {
                auto& t = reg.transforms[i];
                auto& s = reg.sprites[i];
                DrawTextureEx(s.texture, t.position, t.rotation, t.scale.x, s.tint);
            }
        }
    }
}

namespace InputSystem {
    inline void Update(Registry& reg) {
        for (Entity i = 0; i < MAX_ENTITIES; i++) {
            if (reg.HasComponent(i, COMP_INPUT)) {
                auto& input = reg.inputComponents[i];

                input.up = IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
                input.down = IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
                input.left = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
                input.right = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
            }
        }
    }
}

namespace ControlSystem {
    inline void Update(Registry& reg) {
        for (Entity i = 0; i < MAX_ENTITIES; i++) {
            if (reg.HasComponent(i, COMP_INPUT) && reg.HasComponent(i, COMP_VELOCITY)) {
                auto& input = reg.inputComponents[i];
                auto& vel = reg.velocities[i];
                float speed = 300.0f;

                vel.speed = { 0,0 };

                if (input.up)    vel.speed.y = -speed;
                if (input.down)  vel.speed.y = speed;
                if (input.left)  vel.speed.x = -speed;
                if (input.right) vel.speed.x = speed;
            }
        }
    }
}

namespace UISystem {
    inline void UpdateAndDraw(Registry& reg) {
        float sw = (float)GetScreenWidth();
        float sh = (float)GetScreenHeight();

        for (Entity i = 0; i < MAX_ENTITIES; i++) {
            if (reg.HasComponent(i, COMP_UI)) {
                auto& ui = reg.uiComponents[i];

                // 1. Calculate Position based on Anchor
                Vector2 finalPos = { 0, 0 };
                switch (ui.anchor) {
                case ANCHOR_TOP_LEFT:     finalPos = { ui.offset.x, ui.offset.y }; break;
                case ANCHOR_TOP_RIGHT:    finalPos = { sw - ui.size.x - ui.offset.x, ui.offset.y }; break;
                    // TODO - other anchors
                }

                Rectangle rect = { finalPos.x, finalPos.y, ui.size.x, ui.size.y };

                // 2. Collision Detection
                bool isHovered = CheckCollisionPointRec(GetMousePosition(), rect);

                if (isHovered) {
                    // Only trigger if hovered AND clicked
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        ui.isPressed = true;
                        if (ui.onClick) {
                            ui.onClick(); // This will now only fire if the mouse is on the button
                        }
                    }
                    DrawRectangleRec(rect, ColorBrightness(ui.color, -0.2f)); // Hover visual
                }
                else {
                    ui.isPressed = false;
                    DrawRectangleRec(rect, ui.color);
                }

                DrawText(ui.text.c_str(), (int)finalPos.x + 5, (int)finalPos.y + 5, 20, WHITE);
            }
        }
    }
}


namespace EditorSystem {
    inline void DrawAssetBrowser(const std::vector<AssetEntry>& allAssets, std::string& currentPath, int& draggedAssetIndex) {
        float currentSH = (float)GetScreenHeight();
		float currentSW = (float)GetScreenWidth();
        float width = 250;
        float footerHeight = 30;

        // Sidebar Background & Border
        DrawRectangle(0, 0, width, currentSH, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawLine(width, 0, width, currentSH, Fade(BLACK, 0.5f));

        //  Navigation Header (Breadcrumbs)
        DrawRectangle(0, 0, width, 40, Fade(BLACK, 0.2f));
        if (currentPath != "assets") {
            if (GuiButton({ 5, 10, 30, 20 }, "#01#")) {
                size_t lastSlash = currentPath.find_last_of("/\\");
                currentPath = (lastSlash != std::string::npos) ? currentPath.substr(0, lastSlash) : "assets";
            }
        }
        DrawText(GetFileName(currentPath.c_str()), 45, 15, 10, GOLD);

        //  Scrolling Area Setup
        float startY = 40;
        static Vector2 scroll = { 0, 0 };
        Rectangle viewArea = { 0, startY, width, currentSH - startY - footerHeight };

        // Filter logic: Only show direct children of currentPath
        std::vector<int> visibleIndices;
        for (int i = 0; i < (int)allAssets.size(); i++) {
            std::string parent = fs::path(allAssets[i].path).parent_path().string();
            std::replace(parent.begin(), parent.end(), '\\', '/');
            std::string normalizedCurrent = currentPath;
            std::replace(normalizedCurrent.begin(), normalizedCurrent.end(), '\\', '/');

            if (parent == normalizedCurrent) visibleIndices.push_back(i);
        }

        Rectangle content = { 0, 0, width - 20, (float)visibleIndices.size() * 50 };
        Rectangle view = GuiScrollPanel(viewArea, NULL, content, &scroll);

        //  Rendering & Drag Interaction
        BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);
        for (int i = 0; i < (int)visibleIndices.size(); i++) {
            int assetIdx = visibleIndices[i];
            const auto& asset = allAssets[assetIdx];

            float itemY = view.y + scroll.y + (i * 50);
            Rectangle slot = { 5, itemY, width - 25, 45 };

            if (itemY + 45 > view.y && itemY < view.y + view.height) {
                bool hovered = CheckCollisionPointRec(GetMousePosition(), slot);
                DrawRectangleRec(slot, hovered ? Fade(GOLD, 0.2f) : Fade(GRAY, 0.1f));

                // Logic: Click Folder to Navigate, Press/Hold Texture to Drag
                if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (asset.isFolder) {
                        currentPath = asset.path;
                    }
                    else if (asset.isTexture) {
                        draggedAssetIndex = assetIdx; // Start Dragging
                    }
                }

                // Draw Icons
                if (asset.isFolder) {
                    DrawText("#05#", (int)slot.x + 10, (int)slot.y + 12, 20, GOLD);
                }
                else if (asset.isTexture) {
                    DrawTexturePro(asset.preview, { 0, 0, 40, 40 }, { slot.x + 5, slot.y + 5, 35, 35 }, { 0,0 }, 0, WHITE);
                }
                else {
                    DrawText("#12#", (int)slot.x + 10, (int)slot.y + 12, 20, LIGHTGRAY);
                }

                DrawText(asset.name.c_str(), (int)slot.x + 50, (int)slot.y + 15, 12, hovered ? WHITE : LIGHTGRAY);
            }
        }
        EndScissorMode();

        // Drag Visualizer
        if (draggedAssetIndex != -1) {
            Vector2 mPos = GetMousePosition();
            DrawTextureEx(allAssets[draggedAssetIndex].preview, { mPos.x - 20, mPos.y - 20 }, 0, 1.0f, Fade(WHITE, 0.7f));

            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                // Drop logic will be handled in Engine::Update using this index
                // We keep the index valid for one frame so Update() can see where it dropped
            }
        }

        // Footer
        DrawRectangle(0, currentSH - footerHeight, width, footerHeight, Fade(BLACK, 0.8f));
        DrawText(TextFormat("Files: %d", (int)visibleIndices.size()), 10, (int)currentSH - 20, 10, DARKGRAY);
    }

    inline void DrawGrid(int size, Camera2D camera, int screenWidth, int screenHeight, Color color) {
        // Find the top-left and bottom-right corners of the screen in World Space
        Vector2 topLeft = GetScreenToWorld2D({ 0, 0 }, camera);
        Vector2 bottomRight = GetScreenToWorld2D({ (float)screenWidth, (float)screenHeight }, camera);

        // Calculate the starting line positions by snapping the corners to the grid size
        float startX = floor(topLeft.x / size) * size;
        float startY = floor(topLeft.y / size) * size;
        float endX = ceil(bottomRight.x / size) * size;
        float endY = ceil(bottomRight.y / size) * size;

        // Draw Vertical Lines
        for (float x = startX; x <= endX; x += size) {
            DrawLineEx({ x, startY }, { x, endY }, 1.0f / camera.zoom, color);
        }

        // Draw Horizontal Lines
        for (float y = startY; y <= endY; y += size) {
            DrawLineEx({ startX, y }, { endX, y }, 1.0f / camera.zoom, color);
        }
    }

    inline void DrawInspector(Entity e, Registry& reg, int screenWidth, int screenHeight) {
        // Safety Check: -1 or anything beyond our max entities will crash the vector
        if (e < 0 || e >= MAX_ENTITIES) return;

        // Also check if the entity actually exists (has a mask)
        if (reg.entityMasks[e].none()) return;

        float width = 250;
        float x = (float)screenWidth - width;
        float padding = 10;
        float controlHeight = 24;
        float y = 40;

        // Draw Panel Background
        DrawRectangle(x, 0, width, (float)screenHeight, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawLine(x, 0, x, screenHeight, DARKGRAY);

        GuiLabel({ x + padding, 10, width, 30 }, TextFormat("INSPECTOR (Entity %i)", e));

        //  Transform Component
        if (reg.HasComponent(e, COMP_TRANSFORM)) {
            auto& t = reg.transforms[e];

            GuiGroupBox({ x + 5, y, width - 10, 110 }, "TRANSFORM");

            // Position X Slider
            GuiLabel({ x + padding, y + 20, 20, controlHeight }, "X");
            t.position.x = GuiSliderBar({ x + 40, y + 20, width - 60, controlHeight }, NULL, TextFormat("%.2f", t.position.x), t.position.x, -2000, 2000);

            // Position Y Slider
            GuiLabel({ x + padding, y + 50, 20, controlHeight }, "Y");
            t.position.y = GuiSliderBar({ x + 40, y + 50, width - 60, controlHeight }, NULL, TextFormat("%.2f", t.position.y), t.position.y, -2000, 2000);

            // Scale Slider
            GuiLabel({ x + padding, y + 80, 40, controlHeight }, "Scale");
            t.scale.x = GuiSliderBar({ x + 50, y + 80, width - 70, controlHeight }, NULL, TextFormat("%.2f", t.scale.x), t.scale.x, 0.1f, 10.0f);
            t.scale.y = t.scale.x; // Keep aspect ratio

            y += 130;
        }

        // Sprite Component
        if (reg.HasComponent(e, COMP_SPRITE)) {
            auto& s = reg.sprites[e];
            GuiGroupBox({ x + 5, y, width - 10, 60 }, "SPRITE TINT");

            // Color is tricky with raygui sliders, let's use a label for now
            // But we can add a button to reset the tint
            if (GuiButton({ x + padding, y + 20, width - 30, controlHeight }, "Reset Tint to White")) {
                s.tint = WHITE;
            }

            y += 80;
        }

        // Action Buttons
        if (GuiButton({ x + 5, (float)screenHeight - 40, width - 10, 30 }, "DELETE ENTITY")) {
            reg.entityMasks[e].reset();
        }   
    }
    inline void DrawSettingsMenu(bool& open, int& activeTab, Registry& reg, Engine* engine);
}

namespace DebugSystem {
    inline void Draw(const Registry& reg, const EngineStats& stats, int screenWidth) {
        if (!stats.visible) return;

        // Draw background panel
        DrawRectangle(screenWidth - 210, 10, 200, 100, Fade(BLACK, 0.8f));
        DrawRectangleLines(screenWidth - 210, 10, 200, 100, GRAY);

        // Header
        DrawText("ENGINE STATS", screenWidth - 200, 20, 10, GOLD);

        // Data
        int fps = GetFPS();
        Color fpsColor = (fps > 55) ? GREEN : (fps > 30 ? YELLOW : RED);

        DrawText(TextFormat("FPS: %i", fps), screenWidth - 200, 40, 16, fpsColor);
        DrawText(TextFormat("Entities: %i", reg.GetAliveEntityCount()), screenWidth - 200, 65, 12, RAYWHITE);
        DrawText(TextFormat("Frame Time: %.3fms", GetFrameTime() * 1000), screenWidth - 200, 80, 12, RAYWHITE);
    }
}