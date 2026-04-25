#pragma once
#include "raylib.h"
#include "raygui.h"
#include "Registry.h"
#include "Core/Profiler.h"
#include "Utils/AssetManager.h"
#include "Managers/ScriptSystem.h"

class Engine;

namespace TilemapSystem {
    inline void Draw(Registry& reg, ResourceManager& assets) {
        for (Entity i : reg.activeEntities) {
            if (reg.HasComponent(i, COMP_TILEMAP) && reg.HasComponent(i, COMP_TRANSFORM)) {
                auto& map = reg.tilemaps[i];
                auto& t = reg.transforms[i];

                if (map.tileSetPath.empty()) continue;
                TileSet* ts = assets.GetTileSet(map.tileSetPath);
                if (!ts || ts->texture.id == 0) continue;

                for (int y = 0; y < map.height; y++) {
                    for (int x = 0; x < map.width; x++) {
                        int idx = y * map.width + x;
                        if (idx >= (int)map.tiles.size()) continue;
                        const auto& tile = map.tiles[idx];
                        if (tile.index < 0 || tile.index >= (int)ts->sourceRects.size()) continue;

                        Rectangle source = ts->sourceRects[tile.index];
                        if (tile.flipX) source.width *= -1;
                        if (tile.flipY) source.height *= -1;

                        Rectangle dest = {
                            t.position.x + x * map.tileSize * t.scale.x,
                            t.position.y + y * map.tileSize * t.scale.y,
                            (float)map.tileSize * t.scale.x,
                            (float)map.tileSize * t.scale.y
                        };

                        DrawTexturePro(ts->texture, source, dest, {0,0}, 0.0f, tile.tint);
                    }
                }
            }
        }
    }

    inline void ApplyRules(TilemapComponent& map, int x, int y, TileSet* ts) {
        if (!ts || x < 0 || x >= map.width || y < 0 || y >= map.height) return;
        int idx = y * map.width + x;
        int currentTileIndex = map.tiles[idx].index;
        if (currentTileIndex == -1) return;

        const TileConfig* config = nullptr;
        for (const auto& c : ts->tileConfigs) {
            if (c.index == currentTileIndex) {
                config = &c;
                break;
            }
        }

        if (!config || !config->isRuleTile || config->rules.empty()) return;

        int dx[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
        int dy[] = { -1,-1, 0, 1, 1, 1, 0, -1 };

        for (const auto& rule : config->rules) {
            bool match = true;
            for (int i = 0; i < 8; i++) {
                int nx = x + dx[i];
                int ny = y + dy[i];
                NeighborCondition cond = rule.neighbors[i];
                if (cond == NeighborCondition::IGNORE) continue;

                bool isOccupied = false;
                if (nx >= 0 && nx < map.width && ny >= 0 && ny < map.height) {
                    isOccupied = (map.tiles[ny * map.width + nx].index != -1);
                }

                if (cond == NeighborCondition::OCCUPIED && !isOccupied) { match = false; break; }
                if (cond == NeighborCondition::EMPTY && isOccupied) { match = false; break; }
            }
            if (match) {
                map.tiles[idx].index = rule.outputIndex;
                return;
            }
        }
    }

    inline void RefreshRules(TilemapComponent& map, int x, int y, TileSet* ts) {
        ApplyRules(map, x, y, ts);
        int dx[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
        int dy[] = { -1,-1, 0, 1, 1, 1, 0, -1 };
        for (int i = 0; i < 8; i++) {
            ApplyRules(map, x + dx[i], y + dy[i], ts);
        }
    }
}

namespace MovementSystem {
    inline void Update(Registry& reg, float dt) {
        for (Entity i :reg.activeEntities) {
            if (reg.HasComponent(i, COMP_TRANSFORM) && reg.HasComponent(i, COMP_VELOCITY)) {
                reg.transforms[i].position.x += reg.velocities[i].speed.x * dt;
                reg.transforms[i].position.y += reg.velocities[i].speed.y * dt;
            }
        }
    }
}

namespace AnimationSystem {
    inline void Update(Registry& reg, float dt) {
        for (Entity i :reg.activeEntities){
            if (reg.HasComponent(i, COMP_SPRITE_ANIMATION) && reg.HasComponent(i, COMP_SPRITE)) {
                auto& anim = reg.spriteAnimations[i];
                if (!anim.isPlaying || anim.currentState.empty()) continue;
                
                auto it = anim.states.find(anim.currentState);
                if (it == anim.states.end()) continue;
                
                const auto& state = it->second;

                anim.elapsedTime += dt;
                if (anim.elapsedTime >= state.frameDuration) {
                    anim.elapsedTime -= state.frameDuration;

                    anim.currentFrame++;
                    if (anim.currentFrame >= anim.columns) {
                        anim.currentFrame = 0;
                        anim.currentRow++;
                    }

                    bool finished = false;
                    if (anim.currentRow > state.endRow || (anim.currentRow == state.endRow && anim.currentFrame > state.endFrame)) {
                        finished = true;
                    }

                    if (finished) {
                        if (state.loop) {
                            anim.currentFrame = state.startFrame;
                            anim.currentRow = state.startRow;
                        } else {
                            anim.currentFrame = state.endFrame;
                            anim.currentRow = state.endRow;
                            anim.isPlaying = false;
                        }
                    }
                }
            }
        }
    }
}

namespace RenderSystem {
    inline void UpdateShaders(Registry& reg, Camera2D& cam) {
        struct LightInfo {
            int type;
            Color color;
            Vector2 position;
            float radius;
            float intensity;
        };
        std::vector<LightInfo> activeLights;
        for (Entity i : reg.activeEntities) {
            if (reg.HasComponent(i, COMP_LIGHT) && reg.HasComponent(i, COMP_TRANSFORM)) {
                auto& l = reg.lights[i];
                auto& t = reg.transforms[i];
                activeLights.push_back({l.type, l.color, t.position, l.radius, l.intensity});
                if (activeLights.size() >= 16) break;
            }
        }

        std::vector<unsigned int> updatedShaders;
        for (Entity i : reg.activeEntities) {
            if (reg.HasComponent(i, COMP_MATERIAL)) {
                auto& mat = reg.materials[i];
                if (mat.shader.id == 0) continue;

                if (std::find(updatedShaders.begin(), updatedShaders.end(), mat.shader.id) == updatedShaders.end()) {
                    int numLightsLoc = GetShaderLocation(mat.shader, "numLights");
                    if (numLightsLoc != -1) {
                        int num = (int)activeLights.size();
                        SetShaderValue(mat.shader, numLightsLoc, &num, SHADER_UNIFORM_INT);
                        
                        for (int j = 0; j < num; j++) {
                            int typeLoc = GetShaderLocation(mat.shader, TextFormat("lights[%i].type", j));
                            int colorLoc = GetShaderLocation(mat.shader, TextFormat("lights[%i].color", j));
                            int posLoc = GetShaderLocation(mat.shader, TextFormat("lights[%i].position", j));
                            int radiusLoc = GetShaderLocation(mat.shader, TextFormat("lights[%i].radius", j));
                            int intensityLoc = GetShaderLocation(mat.shader, TextFormat("lights[%i].intensity", j));

                            SetShaderValue(mat.shader, typeLoc, &activeLights[j].type, SHADER_UNIFORM_INT);
                            float color[4] = { activeLights[j].color.r / 255.0f, activeLights[j].color.g / 255.0f, activeLights[j].color.b / 255.0f, activeLights[j].color.a / 255.0f };
                            SetShaderValue(mat.shader, colorLoc, color, SHADER_UNIFORM_VEC4);
                            SetShaderValue(mat.shader, posLoc, &activeLights[j].position, SHADER_UNIFORM_VEC2);
                            SetShaderValue(mat.shader, radiusLoc, &activeLights[j].radius, SHADER_UNIFORM_FLOAT);
                            SetShaderValue(mat.shader, intensityLoc, &activeLights[j].intensity, SHADER_UNIFORM_FLOAT);
                        }
                    }
                    updatedShaders.push_back(mat.shader.id);
                }
            }
        }
    }

    inline void Draw(Registry& reg) {
        for (Entity i :reg.activeEntities){
            if (reg.HasComponent(i, COMP_TRANSFORM) && reg.HasComponent(i, COMP_SPRITE)) {
                auto& t = reg.transforms[i];
                auto& s = reg.sprites[i];

                bool hasMaterial = reg.HasComponent(i, COMP_MATERIAL);
                if (hasMaterial && reg.materials[i].shader.id != 0) {
                    BeginShaderMode(reg.materials[i].shader);
                }

                float frameWidth = (float)s.texture.width;
                float frameHeight = (float)s.texture.height;
                float frameX = 0.0f;
				float frameY = 0.0f;

                if (reg.HasComponent(i, COMP_SPRITE_ANIMATION)) {
                    auto& anim = reg.spriteAnimations[i];
                    if (anim.columns > 0 && anim.rows > 0) {
                        frameWidth = (float)s.texture.width / anim.columns;
						frameHeight = (float)s.texture.height / anim.rows;
                        frameX = anim.currentFrame * frameWidth;
						frameY = anim.currentRow * frameHeight;
                    }
                }

                Rectangle sourceRec = {
                    frameX,
                    frameY,
                    frameWidth * (s.flipX ? -1.0f : 1.0f),
                    frameHeight
                };

                Rectangle destRec = {
                    t.position.x,
                    t.position.y,
                    frameWidth * t.scale.x,
                    frameHeight * t.scale.y
                };

                Vector2 origin = {
                    s.anchor.x * destRec.width,
                    s.anchor.y * destRec.height
                };

                DrawTexturePro(s.texture, sourceRec, destRec, origin, t.rotation, s.tint);

                if (hasMaterial && reg.materials[i].shader.id != 0) {
                    EndShaderMode();
                }
            }
        }
    }
}

namespace InputSystem {
    inline void Update(Registry& reg) {
        for (Entity i : reg.activeEntities) {
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
        for (Entity i : reg.activeEntities) {
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

//namespace UISystem {
//    inline void UpdateAndDraw(Registry& reg) {
//        float sw = (float)GetScreenWidth();
//        float sh = (float)GetScreenHeight();
//
//        for (Entity i = 0; i < MAX_ENTITIES; i++) {
//            if (reg.HasComponent(i, COMP_UI)) {
//                auto& ui = reg.uiComponents[i];
//
//                // Calculate Position based on Anchor
//                Vector2 finalPos = { 0, 0 };
//                switch (ui.anchor) {
//                case ANCHOR_TOP_LEFT:     finalPos = { ui.offset.x, ui.offset.y }; break;
//                case ANCHOR_TOP_RIGHT:    finalPos = { sw - ui.size.x - ui.offset.x, ui.offset.y }; break;
//                    // TODO - other anchors
//                }
//
//                Rectangle rect = { finalPos.x, finalPos.y, ui.size.x, ui.size.y };
//
//                // Collision Detection
//                bool isHovered = CheckCollisionPointRec(GetMousePosition(), rect);
//
//                if (isHovered) {
//                    // Only trigger if hovered AND clicked
//                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
//                        ui.isPressed = true;
//                        if (ui.onClick) {
//                            ui.onClick(); // This will now only fire if the mouse is on the button
//                        }
//                    }
//                    DrawRectangleRec(rect, ColorBrightness(ui.color, -0.2f)); // Hover visual
//                }
//                else {
//                    ui.isPressed = false;
//                    DrawRectangleRec(rect, ui.color);
//                }
//
//                DrawText(ui.text.c_str(), (int)finalPos.x + 5, (int)finalPos.y + 5, 20, WHITE);
//            }
//        }
//    }
//}





namespace DebugSystem {

    inline void PhysicsDebug(Registry& reg,Camera2D& cam) {
        for (Entity i : reg.activeEntities) {
            if (reg.HasComponent(i, COMP_CIRCLECOLLIDER) && reg.HasComponent(i,COMP_TRANSFORM)) {
				auto& col = reg.circleColliders[i];

                if (col.debugDraw) {
					auto& transform = reg.transforms[i];
					Vector2 center = Vector2Add(transform.position,col.offset);

					Color debugColor = col.isColliding ? RED : GREEN;

					DrawCircleLinesV(center, col.radius, debugColor);

					DrawCircleV(center, 2.0f, debugColor);
                }
            }
            if (reg.HasComponent(i, COMP_BOXCOLLIDER) && reg.HasComponent(i, COMP_TRANSFORM)) {
                auto& col = reg.boxColliders[i];
                if (col.debugDraw) {
                    auto& transform = reg.transforms[i];

                    Vector2 center = Vector2Add(transform.position, col.offset);

                    Vector2 topLeft = {
                        center.x - (col.size.x * 0.5f),
                        center.y - (col.size.y * 0.5f)
                    };

                    Color debugColor = col.isColliding ? RED : GREEN;

                    DrawRectangleLinesEx({ topLeft.x, topLeft.y, col.size.x, col.size.y }, 1.0f, debugColor);

                    DrawCircleV(center, 2.0f, debugColor);
                }
            }
        }
    }

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

