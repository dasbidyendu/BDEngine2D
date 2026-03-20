#pragma once

#include "ECS/Registry.h"
#include "Utils/Logger.h"
#include "raymath.h"
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <sol/sol.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include "Graphics/ShaderBuilder.h"
#include "Managers/ResourceManager.h"

struct ScriptInstance {
  sol::environment env;
  sol::protected_function startFunc;
  sol::protected_function updateFunc;
};

class ScriptEngine {
public:
  sol::state lua;
  // We cache the COMPILED script code separately to avoid re-loading from disk
  std::unordered_map<std::string, sol::protected_function> compiledScripts;
  
  // We store the LIVE instances per (Entity, ScriptPath)
  std::map<std::pair<Entity, std::string>, ScriptInstance> liveInstances;

  Entity currentEntity = -1;
  std::string currentPath = "";

  void Init(Registry &reg, Camera2D *cam, ResourceManager* resManager = nullptr) {
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string,
                       sol::lib::package, sol::lib::table);

    // --- Logger Bindings ---
    lua["Log"] = [](const char *message) {
      Logger::AddLog(LOG_LEVEL_INFO, message);
    };
    lua["LogWarn"] = [](const char *message) {
      Logger::AddLog(LOG_LEVEL_WARNING, message);
    };
    lua["LogError"] = [](const char *message) {
      Logger::AddLog(LOG_LEVEL_ERROR, message);
    };
    lua["LogSuccess"] = [](const char *message) {
      Logger::AddLog(LOG_LEVEL_SUCCESS, message);
    };

    // Override print
    lua["print"] = [](sol::variadic_args args) {
      std::string output;
      for (auto arg : args) {
        output += arg.as<std::string>() + " ";
      }
      Logger::AddLog(LOG_LEVEL_INFO, "[LUA] %s", output.c_str());
    };

    // --- Property Registration ---
    lua["RegisterProperty"] = [this, &reg](std::string name, sol::object defaultValue) {
        if (currentEntity == -1) return;
        if (!reg.HasComponent(currentEntity, COMP_SCRIPT)) return;

        auto& sc = reg.scripts[currentEntity];
        ScriptInstanceData* instInfo = nullptr;
        for (auto& i : sc.instances) {
            if (i.path == currentPath) {
                instInfo = &i;
                break;
            }
        }

        if (!instInfo) return;

        // Check if property already exists
        for (auto& p : instInfo->properties) {
            if (p.name == name) return;
        }

        ScriptProperty p;
        p.name = name;
        if (defaultValue.is<float>()) {
            p.type = PROP_FLOAT;
            p.floatValue = defaultValue.as<float>();
        } else if (defaultValue.is<int>()) {
            p.type = PROP_INT;
            p.intValue = defaultValue.as<int>();
        } else if (defaultValue.is<bool>()) {
            p.type = PROP_BOOL;
            p.boolValue = defaultValue.as<bool>();
        } else if (defaultValue.is<std::string>()) {
            p.type = PROP_STRING;
            p.stringValue = defaultValue.as<std::string>();
        } else if (defaultValue.is<Vector2>()) {
            p.type = PROP_VECTOR2;
            p.vectorValue = defaultValue.as<Vector2>();
        } else if (defaultValue.is<Color>()) {
            p.type = PROP_COLOR;
            p.colorValue = defaultValue.as<Color>();
        }
        instInfo->properties.push_back(p);
        
        // Also set it in the Lua environment immediately
        auto it = liveInstances.find({currentEntity, currentPath});
        if (it != liveInstances.end()) {
            it->second.env[name] = defaultValue;
        }
    };

    // Binding to get property value in Lua
    lua["GetProperty"] = [this, &reg](std::string name) -> sol::object {
        if (currentEntity == -1) return sol::make_object(lua, sol::nil);
        
        auto it = liveInstances.find({currentEntity, currentPath});
        if (it != liveInstances.end()) {
            return it->second.env[name];
        }
        return sol::make_object(lua, sol::nil);
    };

    lua["CreateEntity"] = [&reg]() { return reg.CreateEntity(); };
    lua["DestroyEntity"] = [&reg](Entity e) { reg.DestroyEntity(e); };

    // Register Raylib Vector2
    lua.new_usertype<Vector2>("Vector2", "x", &Vector2::x, "y", &Vector2::y);

    lua.new_usertype<TransformComponent>(
        "Transform",
        // Position Methods
        "GetPosition", [](TransformComponent &t) { return t.position; },
        "SetPosition",
        [](TransformComponent &t, float x, float y) { t.position = {x, y}; },
        "Translate",
        [](TransformComponent &t, float dx, float dy) {
          t.position.x += dx;
          t.position.y += dy;
        },

        // Scale Methods
        "GetScale", [](TransformComponent &t) { return t.scale; }, "SetScale",
        [](TransformComponent &t, float s) { t.scale = {s, s}; }, "SetScaleEx",
        [](TransformComponent &t, float sx, float sy) { t.scale = {sx, sy}; },

        // Rotation
        "GetRotation", [](TransformComponent &t) { return t.rotation; },
        "SetRotation",
        [](TransformComponent &t, float rot) { t.rotation = rot; });

    lua.new_usertype<SpriteComponent>(
        "Sprite", "texturePath", &SpriteComponent::texturePath, "tint",
        &SpriteComponent::tint, "anchor", &SpriteComponent::anchor, "flipX",
        &SpriteComponent::flipX);

    lua.new_usertype<Color>("Color", "r", &Color::r, "g", &Color::g, "b",
                            &Color::b, "a", &Color::a);

    lua["AddSprite"] = [&reg](Entity e, std::string path, float anchorX,
                               float anchorY) {
      if (e < 0 || e >= MAX_ENTITIES)
        return;
      SpriteComponent sp;
      sp.texturePath = path;
      sp.texture = LoadTexture(
          path.c_str()); // Note: In a real game, use an Asset Cache!
      sp.anchor = {anchorX, anchorY};
      sp.tint = WHITE;
      reg.AddComponent(e, sp);
    };

    lua["AddTransform"] = [&reg](Entity e, float x, float y, float sx,
                                 float sy) {
      if (e < 0 || e >= MAX_ENTITIES)
        return;
      TransformComponent tr;
      tr.position = {x, y};
      tr.scale = {sx, sy};
      tr.rotation = 0.0f;
      reg.AddComponent(e, tr);
    };

    lua["SetEntitySize"] = [&reg](Entity e, float targetWidth,
                                  float targetHeight) {
      if (e < 0 || e >= MAX_ENTITIES || !reg.HasComponent(e, COMP_SPRITE) ||
          !reg.HasComponent(e, COMP_TRANSFORM))
        return;

      auto &sprite = reg.sprites[e];
      auto &transform = reg.transforms[e];

      if (sprite.texture.id != 0) {
        transform.scale.x = targetWidth / (float)sprite.texture.width;
        transform.scale.y = targetHeight / (float)sprite.texture.height;
      }
    };
    
    // ----------- NEW BINDINGS FOR SHADERS & LIGHTS -----------
    lua.new_usertype<MaterialComponent>("Material",
        "shaderName", &MaterialComponent::shaderName,
        "color", &MaterialComponent::color
    );

    lua.new_usertype<LightComponent>("Light",
        "type", &LightComponent::type,
        "color", &LightComponent::color,
        "intensity", &LightComponent::intensity,
        "radius", &LightComponent::radius
    );

    lua["AddMaterial"] = [&reg, resManager](Entity e, std::string shaderName) {
        if (e < 0 || e >= MAX_ENTITIES || !resManager) return;
        MaterialComponent mat;
        mat.shaderName = shaderName;
        mat.shader = resManager->GetShader(shaderName);
        reg.AddComponent(e, mat);
    };

    lua["AddLight"] = [&reg](Entity e, int type, Color col, float intensity, float radius) {
        if (e < 0 || e >= MAX_ENTITIES) return;
        LightComponent lit;
        lit.type = type;
        lit.color = col;
        lit.intensity = intensity;
        lit.radius = radius;
        reg.AddComponent(e, lit);
    };

    lua.create_named_table("Graphics");
    lua["Graphics"]["DefineShader"] = [resManager](sol::table defTable) -> bool {
        if (!resManager) return false;
        
        ShaderDef def;
        def.name = defTable.get_or<std::string>("name", "UnnamedShader");
        def.type = defTable.get_or<std::string>("type", "Unlit");
        def.fragmentLogic = defTable.get_or<std::string>("fragment", "");
        def.vertexLogic = defTable.get_or<std::string>("vertex", "");

        sol::table props = defTable["properties"];
        if (props.valid()) {
            for (auto& kv : props) {
                std::string key = kv.first.as<std::string>();
                sol::table propVals = kv.second.as<sol::table>();
                ShaderProperty p;
                p.type = propVals.get_or<std::string>("type", "float");
                p.defaultValue = propVals.get_or<std::string>("default", "");
                def.properties[key] = p;
            }
        }
        
        Shader compiled = ShaderBuilder::GenerateShader(def);
        if (compiled.id != 0) {
            resManager->AddShader(def.name, compiled);
            return true;
        }
        return false;
    };
    // ---------------------------------------------------------

    lua.set_function("MakeColor", [](float r, float g, float b, float a) {
      return Color{(unsigned char)std::clamp(r, 0.0f, 255.0f),
                   (unsigned char)std::clamp(g, 0.0f, 255.0f),
                   (unsigned char)std::clamp(b, 0.0f, 255.0f),
                   (unsigned char)std::clamp(a, 0.0f, 255.0f)};
    });
    lua["DrawCircleWorld"] = [](float x, float y, float radius, Color color) {
      ::DrawCircleLines((int)x, (int)y, radius, color);
    };
    lua["GetTint"] = [&reg](Entity e) -> Color {
      if (e >= 0 && e < MAX_ENTITIES && reg.HasComponent(e, COMP_SPRITE)) {
        return reg.sprites[e].tint;
      }
      return WHITE;
    };
    lua["GetTime"] = []() { return (float)::GetTime(); };
    lua["InvertColor"] = [&reg](Entity e) {
      if (e < 0 || e >= MAX_ENTITIES || !reg.HasComponent(e, COMP_SPRITE))
        return;

      auto &sprite = reg.sprites[e];

      Image img = LoadImage(sprite.texturePath.c_str());
      if (img.data != nullptr) {
        ImageColorInvert(&img);

        UnloadTexture(sprite.texture);
        sprite.texture = LoadTextureFromImage(img);

        UnloadImage(img);
      } else {
        Logger::AddLog(LOG_LEVEL_WARNING,
                       "LUA: Failed to invert color, image path invalid: %s",
                       sprite.texturePath.c_str());
      }
    };

    lua["SetTint"] = [&reg](Entity e, Color color) {
      if (e >= 0 && e < MAX_ENTITIES && reg.HasComponent(e, COMP_SPRITE)) {
        reg.sprites[e].tint = color;
      }
    };
    // High-level Component Access
    lua["GetTransform"] = [&reg](Entity e) -> TransformComponent * {
      if (e >= 0 && e < MAX_ENTITIES && reg.HasComponent(e, COMP_TRANSFORM)) {
        return &reg.transforms[e];
      }

      TraceLog(LOG_WARNING,
               "LUA: Entity %d does not have a TransformComponent!", e);
      return nullptr;
    };
    lua["GetPosition"] = [&reg](Entity e) {
        if (e >= 0 && e < MAX_ENTITIES && reg.HasComponent(e, COMP_TRANSFORM)) {
            auto& t = reg.transforms[e];
            return std::make_tuple(t.position.x, t.position.y);
        }
        return std::make_tuple(0.0f, 0.0f);
    };
    lua["GetPos"] = lua["GetPosition"];

    lua["GetRotation"] = [&reg](Entity e) {
        if (e >= 0 && e < MAX_ENTITIES && reg.HasComponent(e, COMP_TRANSFORM)) {
            return reg.transforms[e].rotation;
        }
        return 0.0f;
    };
    lua["GetScale"] = [&reg](Entity e) {
        if (e >= 0 && e < MAX_ENTITIES && reg.HasComponent(e, COMP_TRANSFORM)) {
            return std::make_tuple(reg.transforms[e].scale.x, reg.transforms[e].scale.y);
        }
        return std::make_tuple(1.0f, 1.0f);
    };

    lua["SetShaderValueFloat"] = [resManager](std::string shaderName, std::string uniformName, float value) {
        if (!resManager) return;
        Shader s = resManager->GetShader(shaderName);
        if (s.id != 0) {
            int loc = GetShaderLocation(s, uniformName.c_str());
            if (loc != -1) {
                SetShaderValue(s, loc, &value, SHADER_UNIFORM_FLOAT);
            }
        }
    };
    
    lua["SetShaderValueColor"] = [resManager](std::string shaderName, std::string uniformName, Color color) {
        if (!resManager) return;
        Shader s = resManager->GetShader(shaderName);
        if (s.id != 0) {
            int loc = GetShaderLocation(s, uniformName.c_str());
            if (loc != -1) {
                float c[4] = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
                SetShaderValue(s, loc, c, SHADER_UNIFORM_VEC4);
            }
        }
    };

    lua["HasTransform"] = [&reg](Entity e) {
      return reg.HasComponent(e, COMP_TRANSFORM);
    };

    lua["ScreenToWorld"] = [cam](float screenX, float screenY) {
      if (cam) {
        // Converts mouse pixels to game-world coordinates based on pan/zoom
        return GetScreenToWorld2D({screenX, screenY}, *cam);
      }
      return Vector2{screenX, screenY};
    };
    lua["GetEntities"] = [&reg, this](sol::variadic_args args) {
      sol::table results = lua.create_table();
      int index = 1;

      // Determine if we are doing a spatial query
      bool spatial = (args.size() >= 3);
      float cx = 0, cy = 0, r = 0;

      if (spatial) {
        cx = args[0];
        cy = args[1];
        r = args[2];
      }

      for (Entity i = 0; i < MAX_ENTITIES; ++i) {
        if (!reg.HasComponent(i, COMP_TRANSFORM))
          continue;

        if (spatial) {
          auto &pos = reg.transforms[i].position;
          float distSq =
              (pos.x - cx) * (pos.x - cx) + (pos.y - cy) * (pos.y - cy);
          if (distSq > (r * r))
            continue; // Outside radius
        }
        results[index++] = i;
      }
      return results;
    };

    lua["GetDistance"] = [&reg](sol::variadic_args args) {
      // Two Entities - GetDistance(e1, e2)
      if (args.size() == 2) {
        Entity e1 = args[0];
        Entity e2 = args[1];

        if (reg.HasComponent(e1, COMP_TRANSFORM) &&
            reg.HasComponent(e2, COMP_TRANSFORM)) {
          return Vector2Distance(reg.transforms[e1].position,
                                 reg.transforms[e2].position);
        }
      }
      // Point to Entity - GetDistance(x, y, e)
      else if (args.size() == 3) {
        float x = args[0];
        float y = args[1];
        Entity e = args[2];

        if (reg.HasComponent(e, COMP_TRANSFORM)) {
          return Vector2Distance({x, y}, reg.transforms[e].position);
        }
      }

      return -1.0f; // Error/Invalid case
    };

    // Utility / Math
    lua["IsKeyDown"] = [](int key) { return ::IsKeyDown(key); };
    lua["GetDeltaTime"] = []() { return ::GetFrameTime(); };

    lua["KEY_W"] = 87;
    lua["KEY_A"] = 65;
    lua["KEY_S"] = 83;
    lua["KEY_D"] = 68;
    lua["KEY_RIGHT"] = 262;
    lua["KEY_LEFT"] = 263;
    lua["KEY_UP"] = 265;
    lua["KEY_DOWN"] = 264;
    lua["KEY_SPACE"] = 32;
    lua["IsMouseButtonPressed"] = [](int button) {
      return ::IsMouseButtonPressed(button);
    };
    lua["GetMousePos"] = []() { return GetMousePosition(); };
    lua["MOUSE_LEFT"] = 0;
  }

  void LoadScript(Entity e, ScriptInstanceData& instInfo) {
      if (instInfo.path.empty()) return;

      // Compile if not already cached
      if (compiledScripts.find(instInfo.path) == compiledScripts.end()) {
          auto load_result = lua.load_file(instInfo.path);
          if (!load_result.valid()) {
              sol::error err = load_result;
              Logger::AddLog(LOG_LEVEL_ERROR, "LUA LOAD ERROR (%s): %s", instInfo.path.c_str(), err.what());
              return;
          }
          compiledScripts[instInfo.path] = load_result;
      }

      // Create a fresh environment for this instance
      sol::environment env(lua, sol::create, lua.globals());
      
      // Hook up functions
      auto& compiled = compiledScripts[instInfo.path];
      env.set_on(compiled);

      currentEntity = e;
      currentPath = instInfo.path;

      // Run top-level (to register properties and define functions)
      auto result = compiled();
      if (!result.valid()) {
          sol::error err = result;
          Logger::AddLog(LOG_LEVEL_ERROR, "LUA EXEC ERROR (%s): %s", instInfo.path.c_str(), err.what());
          currentEntity = -1;
          currentPath = "";
          return;
      }

      ScriptInstance liveInst;
      liveInst.env = env;
      liveInst.startFunc = env["OnStart"];
      liveInst.updateFunc = env["OnUpdate"];
      
      // Prime properties from C++ into Lua environment if they already exist
      for (const auto& p : instInfo.properties) {
          switch (p.type) {
              case PROP_FLOAT: env[p.name] = p.floatValue; break;
              case PROP_INT: env[p.name] = p.intValue; break;
              case PROP_BOOL: env[p.name] = p.boolValue; break;
              case PROP_STRING: env[p.name] = p.stringValue; break;
              case PROP_VECTOR2: env[p.name] = p.vectorValue; break;
              case PROP_COLOR: env[p.name] = p.colorValue; break;
          }
      }

      liveInstances[{e, instInfo.path}] = liveInst;
      currentEntity = -1;
      currentPath = "";
  }

  void Execute(Entity e, ScriptInstanceData& inst, float dt) {
    if (inst.path.empty()) return;

    auto it = liveInstances.find({e, inst.path});
    if (it == liveInstances.end()) {
        LoadScript(e, inst);
        it = liveInstances.find({e, inst.path});
        if (it == liveInstances.end()) return; // Failed to load
    }

    auto &instance = it->second;
    currentEntity = e;
    currentPath = inst.path;

    // Push C++ property values into Lua environment before update
    // This ensures changes from the Inspector are reflected in the script
    for (const auto& p : inst.properties) {
        switch (p.type) {
            case PROP_FLOAT: instance.env[p.name] = p.floatValue; break;
            case PROP_INT: instance.env[p.name] = p.intValue; break;
            case PROP_BOOL: instance.env[p.name] = p.boolValue; break;
            case PROP_STRING: instance.env[p.name] = p.stringValue; break;
            case PROP_VECTOR2: instance.env[p.name] = p.vectorValue; break;
            case PROP_COLOR: instance.env[p.name] = p.colorValue; break;
        }
    }

    // Handle Start
    if (!inst.started) {
        if (instance.startFunc.valid()) {
            auto result = instance.startFunc(e);
            if (!result.valid()) {
                sol::error err = result;
                Logger::AddLog(LOG_LEVEL_ERROR, "LUA START ERROR (%s): %s", inst.path.c_str(), err.what());
            }
        }
        inst.started = true;
    }

    // Handle Update
    if (instance.updateFunc.valid()) {
      auto result = instance.updateFunc(e, dt);
      if (!result.valid()) {
        sol::error err = result;
        Logger::AddLog(LOG_LEVEL_ERROR, "LUA UPDATE ERROR (%s): %s", inst.path.c_str(), err.what());
      }
    }

    currentEntity = -1;
    currentPath = "";
  }
};