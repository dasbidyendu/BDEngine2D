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
    inline void DrawAssetBrowser(const std::vector<AssetEntry>& assets) {
        float width = 250;
        DrawRectangle(0, 0, width, GetScreenHeight(), Fade(DARKGRAY, 0.95f));
        DrawLine(width, 0, width, GetScreenHeight(), BLACK);
        DrawText("ASSETS", 10, 10, 20, GOLD);

        for (int i = 0; i < assets.size(); i++) {
            Rectangle slot = { 5, 50.0f + (i * 50), width - 10, 45 };
            bool hovered = CheckCollisionPointRec(GetMousePosition(), slot);

            DrawRectangleRec(slot, hovered ? GRAY : Color{ 40, 40, 40, 255 });

            if (assets[i].isTexture) {
                DrawTexture(assets[i].preview, slot.x + 2, slot.y + 2, WHITE);
            }

            DrawText(assets[i].name.c_str(), slot.x + 50, slot.y + 15, 12, hovered ? WHITE : LIGHTGRAY);

            if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                TraceLog(LOG_INFO, "Selected Asset: %s", assets[i].name.c_str());
            }
        }
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
        float x = screenWidth - width;
        float padding = 10;
        float controlHeight = 24;
        float y = 40;

        // 1. Draw Panel Background
        DrawRectangle(x, 0, width, screenHeight, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawLine(x, 0, x, screenHeight, DARKGRAY);

        GuiLabel({ x + padding, 10, width, 30 }, TextFormat("INSPECTOR (Entity %i)", e));

        // 2. Transform Component
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

        // 3. Sprite Component
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

        // 4. Action Buttons
        if (GuiButton({ x + 5, (float)screenHeight - 40, width - 10, 30 }, "DELETE ENTITY")) {
            reg.entityMasks[e].reset();
            // Note: We need a way to tell the engine that e is now -1
            // We'll handle this in the next step
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