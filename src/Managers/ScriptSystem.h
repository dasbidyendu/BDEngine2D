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

    void Init(Registry& reg) {
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::package);
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
        lua["IsKeyDown"] = [](int key) { return ::IsKeyDown(key); };

        lua.new_usertype<TransformComponent>("Transform",
            // Direct property access (Low-level)
            "pos", &TransformComponent::position,
            "scale", &TransformComponent::scale,
            "rotation", &TransformComponent::rotation,

            // Helper "virtual" properties
            "x", sol::property(
                [](TransformComponent& t) { return t.position.x; },
                [](TransformComponent& t, float val) { t.position.x = val; }
            ),
            "y", sol::property(
                [](TransformComponent& t) { return t.position.y; },
                [](TransformComponent& t, float val) { t.position.y = val; }
            )
        );

        // Register Raylib Vector2 so Lua understands .x and .y
        lua.new_usertype<Vector2>("Vector2",
            "x", &Vector2::x,
            "y", &Vector2::y
        );

        // High-level Component Access
        lua["GetTransform"] = [&reg](Entity e) -> TransformComponent& {
            return reg.transforms[e];
            };

        // Utility / Math
        lua["IsKeyDown"] = [](int key) { return ::IsKeyDown(key); };
        lua["GetDeltaTime"] = []() { return ::GetFrameTime(); };
    }

    void Execute(Entity e, const std::string& path, float dt) {
        if (path.empty()) return;

        if (scriptCache.find(path) == scriptCache.end()) {
            // Correct way to set environment in sol3
            sol::environment env(lua, sol::create, lua.globals());

            auto load_result = lua.load_file(path);
            if (!load_result.valid()) return;

            sol::protected_function scriptFunc = load_result;

            // Set environment on the function itself
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