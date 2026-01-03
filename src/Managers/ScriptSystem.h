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
        lua["IsKeyDown"] = [](int key) { return ::IsKeyDown(key); };
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