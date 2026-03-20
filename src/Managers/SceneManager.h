#pragma once
#include "ECS/Registry.h"
#include <string>

class Engine;

class SceneManager {
public:
    static void SaveScene(const std::string& filename, Registry& reg);
    static void LoadScene(const std::string& filename, Registry& reg, Engine* engine);

private:
    // --- COMPONENT SAVERS ---
    static void SaveName(Entity e, Registry& reg, std::ostream& out);
    static void SaveTransform(Entity e, Registry& reg, std::ostream& out);
    static void SaveSprite(Entity e, Registry& reg, std::ostream& out);
    static void SaveVelocity(Entity e, Registry& reg, std::ostream& out);
    static void SaveInput(Entity e, Registry& reg, std::ostream& out);
    static void SaveCanvas(Entity e, Registry& reg, std::ostream& out);
    static void SaveUI(Entity e, Registry& reg, std::ostream& out);
    static void SaveScripts(Entity e, Registry& reg, std::ostream& out);
    static void SaveAnimation(Entity e, Registry& reg, std::ostream& out);
    static void SavePhysics(Entity e, Registry& reg, std::ostream& out);
    static void SaveCircleCollider(Entity e, Registry& reg, std::ostream& out);
    static void SaveBoxCollider(Entity e, Registry& reg, std::ostream& out);
    static void SaveMaterial(Entity e, Registry& reg, std::ostream& out);
    static void SaveLight(Entity e, Registry& reg, std::ostream& out);
    static void SaveCamera(Entity e, Registry& reg, std::ostream& out);

    // --- COMPONENT LOADERS ---
    static void LoadName(Entity e, std::istream& in, Registry& reg);
    static void LoadTransform(Entity e, std::istream& in, Registry& reg);
    static void LoadSprite(Entity e, std::istream& in, Registry& reg, Engine* engine);
    static void LoadVelocity(Entity e, std::istream& in, Registry& reg);
    static void LoadInput(Entity e, std::istream& in, Registry& reg);
    static void LoadCanvas(Entity e, std::istream& in, Registry& reg);
    static void LoadUI(Entity e, std::istream& in, Registry& reg);
    static void LoadScripts(Entity e, std::istream& in_line, std::istream& in_file, Registry& reg);
    static void LoadAnimation(Entity e, std::istream& in_line, std::istream& in_file, Registry& reg);
    static void LoadPhysics(Entity e, std::istream& in, Registry& reg);
    static void LoadCircleCollider(Entity e, std::istream& in, Registry& reg);
    static void LoadBoxCollider(Entity e, std::istream& in, Registry& reg);
    static void LoadMaterial(Entity e, std::istream& in, Registry& reg, Engine* engine);
    static void LoadLight(Entity e, std::istream& in, Registry& reg);
    static void LoadCamera(Entity e, std::istream& in, Registry& reg);
};