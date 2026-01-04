#pragma once
#include "raylib.h"
#include "raygui.h"
#include "Registry.h"
#include "Core/Profiler.h"
#include "Utils/AssetManager.h"
#include "Managers/ScriptSystem.h"

class Engine;

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

                // Define source area (Handling Flips)
                Rectangle sourceRec = {
                    0.0f, 0.0f,
                    (float)s.texture.width * (s.flipX ? -1.0f : 1.0f),
                    (float)s.texture.height
                };

                // Define destination area (Position and Scale)
                Rectangle destRec = {
                    t.position.x, t.position.y,
                    (float)s.texture.width * t.scale.x,
                    (float)s.texture.height * t.scale.y
                };

                // We multiply the normalized anchor (0-1) by the scaled dimensions
                Vector2 origin = {
                    s.anchor.x * destRec.width,
                    s.anchor.y * destRec.height
                };

                DrawTexturePro(s.texture, sourceRec, destRec, origin, t.rotation, s.tint);
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

                // Calculate Position based on Anchor
                Vector2 finalPos = { 0, 0 };
                switch (ui.anchor) {
                case ANCHOR_TOP_LEFT:     finalPos = { ui.offset.x, ui.offset.y }; break;
                case ANCHOR_TOP_RIGHT:    finalPos = { sw - ui.size.x - ui.offset.x, ui.offset.y }; break;
                    // TODO - other anchors
                }

                Rectangle rect = { finalPos.x, finalPos.y, ui.size.x, ui.size.y };

                // Collision Detection
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




namespace DebugSystem {
    inline void Draw(const Registry& reg, DebugStats& stats, int screenWidth) {
        if (!stats.visible) return;

        stats.UpdateHistory();

        float padding = 10.0f;
        float panelWidth = 240.0f;
        float panelHeight = 300.0f;
        float x = (float)screenWidth - panelWidth - padding;
        float y = padding;

        // Background
        DrawRectangleRec({ x, y, panelWidth, panelHeight }, Fade(BLACK, 0.9f));
        DrawRectangleLinesEx({ x, y, panelWidth, panelHeight }, 1, DARKGRAY);

        // Header & FPS
        DrawText("ENGINE PROFILER", (int)x + 10, (int)y + 10, 10, GOLD);
        int fps = GetFPS();
        Color fpsColor = (fps > 55) ? GREEN : (fps > 30 ? YELLOW : RED);
        DrawText(TextFormat("%i FPS", fps), (int)x + 10, (int)y + 25, 22, fpsColor);
        DrawText(TextFormat("%.3f ms/frame", GetFrameTime() * 1000), (int)x + 10, (int)y + 50, 13, LIGHTGRAY);

        // --- FPS GRAPH ---
        float graphX = x + 10;
        float graphY = y + 70;
        float graphWidth = panelWidth - 20;
        float graphHeight = 45;
        DrawRectangle(graphX, graphY, graphWidth, graphHeight, Color{ 30, 30, 30, 255 });

        for (int i = 0; i < 59; i++) {
            int currIdx = (stats.historyIndex + i) % 60;
            int nextIdx = (stats.historyIndex + i + 1) % 60;

            float h1 = (stats.fpsHistory[currIdx] / 120.0f) * graphHeight;
            float h2 = (stats.fpsHistory[nextIdx] / 120.0f) * graphHeight;

            h1 = fminf(h1, graphHeight);
            h2 = fminf(h2, graphHeight);

            Vector2 p1 = { graphX + (i * (graphWidth / 60.0f)), graphY + graphHeight - h1 };
            Vector2 p2 = { graphX + ((i + 1) * (graphWidth / 60.0f)), graphY + graphHeight - h2 };
            DrawLineV(p1, p2, LIME);
        }

        // --- WORLD STATS ---
        float statsY = graphY + graphHeight + 15;
        DrawText("WORLD", (int)x + 10, (int)statsY, 10, GOLD);
        DrawRectangle(x + 10, statsY + 12, panelWidth - 20, 1, DARKGRAY);
        DrawText(TextFormat("Entities: %i", reg.GetAliveEntityCount()), (int)x + 10, (int)statsY + 18, 12, RAYWHITE);

        // --- SYSTEM LOAD (Visual Bars) ---
        float profilerY = statsY + 45;
        DrawText("SYSTEM LOAD BREAKDOWN", (int)x + 10, (int)profilerY, 10, GOLD);

        auto DrawMetric = [&](const char* label, float timeMs, Color color, int order) {
            float barY = profilerY + 18 + (order * 22);
            float totalFrameTime = GetFrameTime() * 1000.0f;
            float percentage = (totalFrameTime > 0) ? (timeMs / totalFrameTime) : 0;

            DrawText(label, (int)x + 10, (int)barY, 11, RAYWHITE);
            // Bar Background
            DrawRectangle(x + 85, barY, 110, 12, Color{ 50, 50, 50, 255 });
            // Filled Bar
            DrawRectangle(x + 85, barY, 110 * fminf(percentage, 1.0f), 12, color);
            // Text Value
            DrawText(TextFormat("%.1fms", timeMs), (int)x + 200, (int)barY, 10, LIGHTGRAY);
            };

        // These metrics will work once you wrap your Update calls with timers
        DrawMetric("Core", stats.logicTime, BLUE, 0);
        DrawMetric("Scripts", stats.scriptTime, PURPLE, 1);
        DrawMetric("Render", stats.renderTime, ORANGE, 2);

        DrawText("F1: Settings | F2: Debug", (int)x + 10, (int)y + panelHeight - 15, 10, GRAY);
    }
}