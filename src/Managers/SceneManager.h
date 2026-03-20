#pragma once
#include "ECS/Registry.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

// Forward declaration to avoid circular includes
class Engine;

class SceneManager {
public:
  static void SaveScene(const std::string &filename, Registry &reg) {
    std::ofstream file(filename);
    if (!file.is_open())
      return;

    for (Entity i = 0; i < MAX_ENTITIES; i++) {
      if (reg.entityMasks[i].none())
        continue;

      file << "ENTITY " << i << "\n";

      if (reg.HasComponent(i, COMP_NAME)) {
        file << "NAME " << std::quoted(reg.names[i].name) << "\n";
      }

      if (reg.HasComponent(i, COMP_TRANSFORM)) {
        auto &t = reg.transforms[i];
        file << "TRANSFORM " << t.position.x << " " << t.position.y << " "
             << t.scale.x << " " << t.scale.y << " " << t.rotation << "\n";
      }

      if (reg.HasComponent(i, COMP_SPRITE)) {
        auto &s = reg.sprites[i];
        file << "SPRITE " << std::quoted(s.texturePath) << " " << (int)s.tint.r
             << " " << (int)s.tint.g << " " << (int)s.tint.b << " "
             << (int)s.tint.a << " " << s.anchor.x << " " << s.anchor.y << " "
             << (s.flipX ? 1 : 0) << "\n";
      }

      if (reg.HasComponent(i, COMP_VELOCITY)) {
        auto &v = reg.velocities[i];
        file << "VELOCITY " << v.speed.x << " " << v.speed.y << "\n";
      }

      if (reg.HasComponent(i, COMP_SCRIPT)) {
        auto &sc = reg.scripts[i];
        file << "SCRIPTS " << sc.instances.size() << "\n";
        for (const auto &inst : sc.instances) {
          file << "INSTANCE " << std::quoted(inst.path) << " "
               << inst.properties.size() << "\n";
          for (const auto &p : inst.properties) {
            file << "PROPERTY " << (int)p.type << " " << std::quoted(p.name)
                 << " ";
            switch (p.type) {
            case PROP_FLOAT:
              file << p.floatValue << "\n";
              break;
            case PROP_INT:
              file << p.intValue << "\n";
              break;
            case PROP_BOOL:
              file << (p.boolValue ? 1 : 0) << "\n";
              break;
            case PROP_STRING:
              file << std::quoted(p.stringValue) << "\n";
              break;
            case PROP_VECTOR2:
              file << p.vectorValue.x << " " << p.vectorValue.y << "\n";
              break;
            case PROP_COLOR:
              file << (int)p.colorValue.r << " " << (int)p.colorValue.g << " "
                   << (int)p.colorValue.b << " " << (int)p.colorValue.a << "\n";
              break;
            }
          }
        }
      }

      if (reg.HasComponent(i, COMP_UI)) {
        auto &ui = reg.uiComponents[i];
        file << "UI " << std::quoted(ui.text) << " " << (int)ui.anchor << " "
             << ui.offset.x << " " << ui.offset.y << " " << ui.size.x << " "
             << ui.size.y << " " << (int)ui.color.r << " " << (int)ui.color.g
             << " " << (int)ui.color.b << " " << (int)ui.color.a << "\n";
      }

      if (reg.HasComponent(i, COMP_CAMERA)) {
        auto &cam = reg.cameras[i];
        file << "CAMERA " << cam.zoom << " " << cam.offset.x << " "
             << cam.offset.y << " " << cam.target.x << " " << cam.target.y << " "
             << cam.rotation << " " << (cam.isPrimary ? 1 : 0) << "\n";
      }

      file << "END\n";
    }
    file.close();
  }

  static void LoadScene(const std::string &filename, Registry &reg,
                        Engine *engine) {
    std::ifstream file(filename);
    if (!file.is_open())
      return;

    reg.Clear();
    std::string line;
    Entity maxE = 0;

    Entity currentE = 0;

    while (std::getline(file, line)) {
      std::stringstream ss(line);
      std::string cmd;
      ss >> cmd;

      if (cmd == "ENTITY") {
        ss >> currentE;
        if (currentE >= maxE)
          maxE = currentE + 1;

        // Ensure entity is tracked in activeEntities
        if (std::find(reg.activeEntities.begin(), reg.activeEntities.end(),
                      currentE) == reg.activeEntities.end()) {
          reg.activeEntities.push_back(currentE);
        }
      } else if (cmd == "NAME") {
        NameComponent n;
        ss >> std::quoted(n.name);
        reg.AddComponent(currentE, n);
      } else if (cmd == "TRANSFORM") {
        TransformComponent t;
        ss >> t.position.x >> t.position.y >> t.scale.x >> t.scale.y >>
            t.rotation;
        t.padding = 0.0f;
        reg.AddComponent(currentE, t);
      } else if (cmd == "SPRITE") {
        SpriteComponent s;
        int r, g, b, a, flip;
        ss >> std::quoted(s.texturePath) >> r >> g >> b >> a >> s.anchor.x >>
            s.anchor.y >> flip;
        s.tint = {(unsigned char)r, (unsigned char)g, (unsigned char)b,
                  (unsigned char)a};
        s.flipX = (flip == 1);

        if (!s.texturePath.empty()) {
          // Note: accessing engine->assets directly as per your class header
          engine->assets.LoadTextureAsset(s.texturePath, s.texturePath);
          s.texture = engine->assets.GetTexture(s.texturePath);
        }
        reg.AddComponent(currentE, s);
      } else if (cmd == "VELOCITY") {
        VelocityComponent v;
        ss >> v.speed.x >> v.speed.y;
        v.pad[0] = 0.0f;
        v.pad[1] = 0.0f;
        reg.AddComponent(currentE, v);
      } else if (cmd == "SCRIPTS_LEGACY") {
        int count;
        ss >> count;
        ScriptComponent sc;
        for (int j = 0; j < count; j++) {
          std::string p;
          ss >> std::quoted(p);
          ScriptInstanceData inst;
          inst.path = p;
          sc.instances.push_back(inst);
        }
        reg.AddComponent(currentE, sc);
      } else if (cmd == "SCRIPTS") {
        int instCount;
        ss >> instCount;
        ScriptComponent sc;
        for (int j = 0; j < instCount; j++) {
          std::string instLine;
          std::getline(file, instLine);
          std::stringstream ssInst(instLine);
          std::string instCmd;
          ssInst >> instCmd;
          if (instCmd == "INSTANCE") {
            ScriptInstanceData inst;
            int propCount;
            ssInst >> std::quoted(inst.path) >> propCount;
            for (int k = 0; k < propCount; k++) {
              std::string propLine;
              std::getline(file, propLine);
              std::stringstream ssProp(propLine);
              std::string propCmd;
              ssProp >> propCmd;
              if (propCmd == "PROPERTY") {
                ScriptProperty p;
                int type;
                ssProp >> type >> std::quoted(p.name);
                p.type = (ScriptPropertyType)type;
                switch (p.type) {
                  case PROP_FLOAT: ssProp >> p.floatValue; break;
                  case PROP_INT: ssProp >> p.intValue; break;
                  case PROP_BOOL: { int b; ssProp >> b; p.boolValue = (b == 1); } break;
                  case PROP_STRING: ssProp >> std::quoted(p.stringValue); break;
                  case PROP_VECTOR2: ssProp >> p.vectorValue.x >> p.vectorValue.y; break;
                  case PROP_COLOR: { int r, g, b, a; ssProp >> r >> g >> b >> a; p.colorValue = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a}; } break;
                }
                inst.properties.push_back(p);
              }
            }
            sc.instances.push_back(inst);
          }
        }
        reg.AddComponent(currentE, sc);
      } else if (cmd == "UI") {
        UIComponent ui;
        int anchor, r, g, b, a;
        ss >> std::quoted(ui.text) >> anchor >> ui.offset.x >> ui.offset.y >>
            ui.size.x >> ui.size.y >> r >> g >> b >> a;
        ui.anchor = (UIAnchor)anchor;
        ui.color = {(unsigned char)r, (unsigned char)g, (unsigned char)b,
                    (unsigned char)a};
        reg.AddComponent(currentE, ui);
      } else if (cmd == "CAMERA") {
        CameraComponent cam;
        int primary;
        ss >> cam.zoom >> cam.offset.x >> cam.offset.y >> cam.target.x >>
            cam.target.y >> cam.rotation >> primary;
        cam.isPrimary = (primary == 1);
        reg.AddComponent(currentE, cam);
      }
    }
    reg.SetNextEntity(maxE);
    file.close();
  }
};