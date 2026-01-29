#pragma once

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <sol/sol.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include "ECS/Registry.h"

struct ScriptInstance {
    sol::environment env;
    sol::protected_function updateFunc;
};

class ScriptEngine {
public:
    sol::state lua;
    std::unordered_map<std::string, ScriptInstance> scriptCache;

    void Init(Registry& reg,Camera2D* cam) {
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::package);

		lua["CreateEntity"] = [&reg]() {
            return reg.CreateEntity();
            };

        //Direct access methods
        lua["GetPos"] = [&reg](Entity e) {
            auto& pos = reg.transforms[e].position;
            return std::make_tuple(pos.x, pos.y);
            };
        lua["SetPos"] = [&reg](Entity e, float x, float y) {
            reg.transforms[e].position = { x, y };
            };
        lua["GetRotation"] = [&reg](Entity e) {
			auto& rot = reg.transforms[e].rotation;
            return rot;
            };
        lua["SetRotation"] = [&reg](Entity e, float angle) {
            reg.transforms[e].rotation = angle;
            };
        lua["SetScale"] = [&reg](Entity e, float s) {
            reg.transforms[e].scale = { s, s };
            };


        // Register Raylib Vector2
        lua.new_usertype<Vector2>("Vector2",
            "x", &Vector2::x,
            "y", &Vector2::y
        );

        lua.new_usertype<TransformComponent>("Transform",
            // Position Methods
            "GetPosition", [](TransformComponent& t) { return t.position; },
            "SetPosition", [](TransformComponent& t, float x, float y) { t.position = { x, y }; },
            "Translate", [](TransformComponent& t, float dx, float dy) { t.position.x += dx; t.position.y += dy; },

            // Scale Methods
            "GetScale", [](TransformComponent& t) { return t.scale; },
            "SetScale", [](TransformComponent& t, float s) { t.scale = { s, s }; },
            "SetScaleEx", [](TransformComponent& t, float sx, float sy) { t.scale = { sx, sy }; },

            // Rotation
            "GetRotation", [](TransformComponent& t) { return t.rotation; },
            "SetRotation", [](TransformComponent& t, float rot) { t.rotation = rot; }
        );

        lua.new_usertype<SpriteComponent>("Sprite",
            "texturePath", &SpriteComponent::texturePath,
            "tint", &SpriteComponent::tint,
            "anchor", &SpriteComponent::anchor,
            "flipX", &SpriteComponent::flipX
		);

        lua.new_usertype<Color>("Color",
            "r", &Color::r,
            "g", &Color::g,
            "b", &Color::b,
            "a", &Color::a
		);

        lua["AddSprite"] = [&reg](Entity e, std::string path, float anchorX, float anchorY) {
            if (e >= MAX_ENTITIES) return;
            SpriteComponent sp;
            sp.texturePath = path;
            sp.texture = LoadTexture(path.c_str()); // Note: In a real game, use an Asset Cache!
            sp.anchor = { anchorX, anchorY };
            sp.tint = WHITE;
            reg.AddComponent(e, sp);
            };

        lua["AddTransform"] = [&reg](Entity e, float x, float y, float sx, float sy) {
            if (e >= MAX_ENTITIES) return;
            TransformComponent tr;
            tr.position = { x, y };
            tr.scale = { sx, sy };
            tr.rotation = 0.0f;
            reg.AddComponent(e, tr);
            };

        lua["SetEntitySize"] = [&reg](Entity e, float targetWidth, float targetHeight) {
            if (e >= MAX_ENTITIES || !reg.HasComponent(e, COMP_SPRITE) || !reg.HasComponent(e, COMP_TRANSFORM)) return;

            auto& sprite = reg.sprites[e];
            auto& transform = reg.transforms[e];

            if (sprite.texture.id != 0) {
                transform.scale.x = targetWidth / (float)sprite.texture.width;
                transform.scale.y = targetHeight / (float)sprite.texture.height;
            }
            };

        lua.set_function("MakeColor", [](unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
            return Color{ r, g, b, a };
            });

        lua["InvertColor"] = [&reg](Entity e) {
            if (e >= MAX_ENTITIES || !reg.HasComponent(e, COMP_SPRITE)) return;

            auto& sprite = reg.sprites[e];

            Image img = LoadImage(sprite.texturePath.c_str());
            if (img.data != nullptr) {
                ImageColorInvert(&img);

                UnloadTexture(sprite.texture);
                sprite.texture = LoadTextureFromImage(img);

                UnloadImage(img);
            }
            else {
                TraceLog(LOG_WARNING, "LUA: Failed to invert color, image path invalid: %s", sprite.texturePath.c_str());
            }
            };

        lua["SetTint"] = [&reg](Entity e, Color color) {
            if (e < MAX_ENTITIES && reg.HasComponent(e, COMP_SPRITE)) {
                reg.sprites[e].tint = color;
            }
            };
        // High-level Component Access
        lua["GetTransform"] = [&reg](Entity e) -> TransformComponent* {
            if (e >= 0 && e < MAX_ENTITIES && reg.HasComponent(e, COMP_TRANSFORM)) {
                return &reg.transforms[e];
            }

            TraceLog(LOG_WARNING, "LUA: Entity %d does not have a TransformComponent!", e);
            return nullptr;
            };
        lua["HasTransform"] = [&reg](Entity e) {
            return reg.HasComponent(e, COMP_TRANSFORM);
            };

        lua["ScreenToWorld"] = [cam](float screenX, float screenY) {
            if (cam) {
                // Converts mouse pixels to game-world coordinates based on pan/zoom
                return GetScreenToWorld2D({ screenX, screenY }, *cam);
            }
            return Vector2{ screenX, screenY };
            };

        // Utility / Math
        lua["IsKeyDown"] = [](int key) { return ::IsKeyDown(key); };
        lua["GetDeltaTime"] = []() { return ::GetFrameTime(); };

        lua["KEY_W"] = 87; lua["KEY_A"] = 65; lua["KEY_S"] = 83; lua["KEY_D"] = 68;
        lua["KEY_RIGHT"] = 262; lua["KEY_LEFT"] = 263; lua["KEY_UP"] = 265; lua["KEY_DOWN"] = 264;
        lua["KEY_SPACE"] = 32;
        lua["IsMouseButtonPressed"] = [](int button) { return ::IsMouseButtonPressed(button); };
        lua["GetMousePos"] = []() {
            return GetMousePosition();
            };
        lua["MOUSE_LEFT"] = 0;
    }

    void Execute(Entity e, const std::string& path, float dt) {
        if (path.empty()) return;

        if (scriptCache.find(path) == scriptCache.end()) {
            sol::environment env(lua, sol::create, lua.globals());

            auto load_result = lua.load_file(path);
            if (!load_result.valid()) return;

            sol::protected_function scriptFunc = load_result;

            env.set_on(scriptFunc);

            auto result = scriptFunc();
            if (result.valid()) {
                sol::protected_function func = env["OnUpdate"];
                scriptCache[path] = { env, func };
            }
        }

        auto& instance = scriptCache[path];
        if (instance.updateFunc.valid()) {
            auto result = instance.updateFunc(e, dt);
            if (!result.valid()) {
                sol::error err = result;
                TraceLog(LOG_WARNING, "LUA ERROR (%s): %s", path.c_str(), err.what());
            }
        }
    }
};