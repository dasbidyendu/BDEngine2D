#pragma once
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "ECS/Registry.h"
#include "Engine.h"

class SceneManager {
public:
    static void SaveScene(const std::string& filename, Registry& reg) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cout << "Failed to open file for saving: " << filename << std::endl;
            return;
        }

        for (Entity i = 0; i < MAX_ENTITIES; i++) {
            if (reg.entityMasks[i].none()) continue;

            file << "ENTITY " << i << "\n";

            if (reg.HasComponent(i, COMP_TRANSFORM)) {
                auto& t = reg.transforms[i];
                file << "TRANSFORM " << t.position.x << " " << t.position.y << " "
                    << t.scale.x << " " << t.scale.y << " " << t.rotation << "\n";
            }

            if (reg.HasComponent(i, COMP_SPRITE)) {
                auto& s = reg.sprites[i];
                file << "SPRITE " << std::quoted(s.texturePath) << " "
                    << (int)s.tint.r << " " << (int)s.tint.g << " " << (int)s.tint.b << " " << (int)s.tint.a << " "
                    << s.anchor.x << " " << s.anchor.y << " " << (s.flipX ? 1 : 0) << "\n";
            }

            if (reg.HasComponent(i, COMP_VELOCITY)) {
                auto& v = reg.velocities[i];
                file << "VELOCITY " << v.speed.x << " " << v.speed.y << "\n";
            }

            if (reg.HasComponent(i, COMP_INPUT)) {
                // Assuming InputComponent might have a simple toggle or type
                file << "INPUT\n";
            }

            if (reg.HasComponent(i, COMP_SCRIPT)) {
                auto& sc = reg.scripts[i];
                file << "SCRIPTS " << sc.scriptPaths.size();
                for (const auto& path : sc.scriptPaths) {
                    file << " " << std::quoted(path);
                }
                file << "\n";
            }

            /*if (reg.HasComponent(i, COMP_UICANVAS)) {
                auto& canv = reg.uiCanvases[i];
                file << "UICANVAS " << (canv.isVisible ? 1 : 0) << "\n";
            }*/

            if (reg.HasComponent(i, COMP_UI)) {
                auto& ui = reg.uiComponents[i];
                file << "UI " << std::quoted(ui.text) << " " << (int)ui.anchor << " "
                    << ui.offset.x << " " << ui.offset.y << " " << ui.size.x << " " << ui.size.y << " "
                    << (int)ui.color.r << " " << (int)ui.color.g << " " << (int)ui.color.b << " " << (int)ui.color.a << "\n";
            }

            file << "END\n";
        }
        file.close();
    }

    static void LoadScene(const std::string& filename, Registry& reg, Engine* engine) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Failed to open scene: " << filename << std::endl;
            return;
        }

        reg.Clear();
        std::string line;
        Entity maxE = 0;
        Entity currentE = 0;

        while (std::getline(file, line)) {
            if (line.empty() || line == "END") continue;

            std::stringstream ss(line);
            std::string cmd;
            ss >> cmd;

            if (cmd == "ENTITY") {
                ss >> currentE;
                if (currentE >= maxE) maxE = currentE + 1;
            }
            else if (cmd == "TRANSFORM") {
                TransformComponent t;
                ss >> t.position.x >> t.position.y >> t.scale.x >> t.scale.y >> t.rotation;
                reg.AddComponent(currentE, t);
            }
            else if (cmd == "SPRITE") {
                SpriteComponent s;
                int r, g, b, a, flip;
                ss >> std::quoted(s.texturePath) >> r >> g >> b >> a >> s.anchor.x >> s.anchor.y >> flip;
                s.tint = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
                s.flipX = (flip == 1);

                if (!s.texturePath.empty()) {
                    s.texture = engine->assets.GetTexture(s.texturePath);
                    // If not in manager, load it (using name as path for consistency)
                    if (s.texture.id == 0) {
                        engine->assets.LoadTextureAsset(s.texturePath, s.texturePath);
                        s.texture = engine->assets.GetTexture(s.texturePath);
                    }
                }
                reg.AddComponent(currentE, s);
            }
            else if (cmd == "VELOCITY") {
                VelocityComponent v;
                ss >> v.speed.x >> v.speed.y;
                reg.AddComponent(currentE, v);
            }
            else if (cmd == "INPUT") {
                reg.AddComponent(currentE, InputComponent{});
            }
            else if (cmd == "SCRIPTS") {
                int count;
                ss >> count;
                ScriptComponent sc;
                for (int j = 0; j < count; j++) {
                    std::string p;
                    ss >> std::quoted(p);
                    sc.scriptPaths.push_back(p);
                }
                reg.AddComponent(currentE, sc);
            }
            /*else if (cmd == "UICANVAS") {
                int visible;
                ss >> visible;
                reg.AddComponent(currentE, UICanvasComponent{ (bool)visible });
            }*/
            else if (cmd == "UI") {
                UIComponent ui;
                int anchor, r, g, b, a;
                ss >> std::quoted(ui.text) >> anchor >> ui.offset.x >> ui.offset.y >> ui.size.x >> ui.size.y >> r >> g >> b >> a;
                ui.anchor = (UIAnchor)anchor;
                ui.color = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
                reg.AddComponent(currentE, ui);
            }
        }

        reg.SetNextEntity(maxE);
        file.close();

        std::cout << "Scene Loaded: " << filename << " (Entities: " << (int)maxE << ")" << std::endl;
    }
};