#include "Managers/SceneManager.h"
#include "Core/Engine.h"
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

void SceneManager::SaveScene(const std::string &filename, Registry &reg) {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    for (Entity i = 0; i < MAX_ENTITIES; i++) {
        if (reg.entityMasks[i].none()) continue;

        file << "ENTITY " << i << "\n";

        if (reg.HasComponent(i, COMP_NAME)) SaveName(i, reg, file);
        if (reg.HasComponent(i, COMP_TRANSFORM)) SaveTransform(i, reg, file);
        if (reg.HasComponent(i, COMP_SPRITE)) SaveSprite(i, reg, file);
        if (reg.HasComponent(i, COMP_VELOCITY)) SaveVelocity(i, reg, file);
        if (reg.HasComponent(i, COMP_INPUT)) SaveInput(i, reg, file);
        if (reg.HasComponent(i, COMP_UICANVAS)) SaveCanvas(i, reg, file);
        if (reg.HasComponent(i, COMP_UI)) SaveUI(i, reg, file);
        if (reg.HasComponent(i, COMP_SCRIPT)) SaveScripts(i, reg, file);
        if (reg.HasComponent(i, COMP_SPRITE_ANIMATION)) SaveAnimation(i, reg, file);
        if (reg.HasComponent(i, COMP_RIGIDPHYSICS)) SavePhysics(i, reg, file);
        if (reg.HasComponent(i, COMP_CIRCLECOLLIDER)) SaveCircleCollider(i, reg, file);
        if (reg.HasComponent(i, COMP_BOXCOLLIDER)) SaveBoxCollider(i, reg, file);
        if (reg.HasComponent(i, COMP_MATERIAL)) SaveMaterial(i, reg, file);
        if (reg.HasComponent(i, COMP_LIGHT)) SaveLight(i, reg, file);
        if (reg.HasComponent(i, COMP_CAMERA)) SaveCamera(i, reg, file);
        if (reg.HasComponent(i, COMP_TILEMAP)) SaveTilemap(i, reg, file);

        file << "END\n";
    }
    file.close();
}

void SceneManager::LoadScene(const std::string &filename, Registry &reg, Engine *engine) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    reg.Clear();
    std::string line;
    Entity maxE = 0;
    Entity currentE = 0;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "ENTITY") {
            ss >> currentE;
            if (currentE >= maxE) maxE = currentE + 1;
            if (std::find(reg.activeEntities.begin(), reg.activeEntities.end(), currentE) == reg.activeEntities.end()) {
                reg.activeEntities.push_back(currentE);
            }
        } 
        else if (cmd == "NAME") LoadName(currentE, ss, reg);
        else if (cmd == "TRANSFORM") LoadTransform(currentE, ss, reg);
        else if (cmd == "SPRITE") LoadSprite(currentE, ss, reg, engine);
        else if (cmd == "VELOCITY") LoadVelocity(currentE, ss, reg);
        else if (cmd == "INPUT") LoadInput(currentE, ss, reg);
        else if (cmd == "CANVAS") LoadCanvas(currentE, ss, reg);
        else if (cmd == "UI") LoadUI(currentE, ss, reg);
        else if (cmd == "SCRIPTS") LoadScripts(currentE, ss, file, reg);
        else if (cmd == "ANIMATION") LoadAnimation(currentE, ss, file, reg);
        else if (cmd == "PHYSICS") LoadPhysics(currentE, ss, reg);
        else if (cmd == "CIRCLE_COLLIDER") LoadCircleCollider(currentE, ss, reg);
        else if (cmd == "BOX_COLLIDER") LoadBoxCollider(currentE, ss, reg);
        else if (cmd == "MATERIAL") LoadMaterial(currentE, ss, reg, engine);
        else if (cmd == "LIGHT") LoadLight(currentE, ss, reg);
        else if (cmd == "CAMERA") LoadCamera(currentE, ss, reg);
        else if (cmd == "TILEMAP") LoadTilemap(currentE, ss, file, reg, engine);
    }
    reg.SetNextEntity(maxE);
    file.close();
}

// --- NAME ---
void SceneManager::SaveName(Entity e, Registry &reg, std::ostream &out) {
    out << "NAME " << std::quoted(reg.names[e].name) << "\n";
}
void SceneManager::LoadName(Entity e, std::istream &in, Registry &reg) {
    NameComponent c;
    in >> std::quoted(c.name);
    reg.AddComponent(e, c);
}

// --- TRANSFORM ---
void SceneManager::SaveTransform(Entity e, Registry &reg, std::ostream &out) {
    auto &t = reg.transforms[e];
    out << "TRANSFORM " << t.position.x << " " << t.position.y << " "
        << t.scale.x << " " << t.scale.y << " " << t.rotation << "\n";
}
void SceneManager::LoadTransform(Entity e, std::istream &in, Registry &reg) {
    TransformComponent t;
    in >> t.position.x >> t.position.y >> t.scale.x >> t.scale.y >> t.rotation;
    t.padding = 0;
    reg.AddComponent(e, t);
}

// --- SPRITE ---
void SceneManager::SaveSprite(Entity e, Registry &reg, std::ostream &out) {
    auto &s = reg.sprites[e];
    out << "SPRITE " << std::quoted(s.texturePath) << " " << (int)s.tint.r
        << " " << (int)s.tint.g << " " << (int)s.tint.b << " " << (int)s.tint.a 
        << " " << s.anchor.x << " " << s.anchor.y << " " << (s.flipX ? 1 : 0) << "\n";
}
void SceneManager::LoadSprite(Entity e, std::istream &in, Registry &reg, Engine *engine) {
    SpriteComponent s;
    int r, g, b, a, flip;
    in >> std::quoted(s.texturePath) >> r >> g >> b >> a >> s.anchor.x >> s.anchor.y >> flip;
    s.tint = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
    s.flipX = (flip == 1);
    
    if (!s.texturePath.empty() && engine) {
        engine->assets.LoadTextureAsset(s.texturePath, s.texturePath);
        s.texture = engine->assets.GetTexture(s.texturePath);
    }
    reg.AddComponent(e, s);
}

// --- VELOCITY ---
void SceneManager::SaveVelocity(Entity e, Registry &reg, std::ostream &out) {
    auto &v = reg.velocities[e];
    out << "VELOCITY " << v.speed.x << " " << v.speed.y << "\n";
}
void SceneManager::LoadVelocity(Entity e, std::istream &in, Registry &reg) {
    VelocityComponent v;
    in >> v.speed.x >> v.speed.y;
    reg.AddComponent(e, v);
}

// --- INPUT ---
void SceneManager::SaveInput(Entity e, Registry &reg, std::ostream &out) {
    out << "INPUT\n";
}
void SceneManager::LoadInput(Entity e, std::istream &in, Registry &reg) {
    reg.AddComponent(e, InputComponent{});
}

// --- CANVAS ---
void SceneManager::SaveCanvas(Entity e, Registry &reg, std::ostream &out) {
    auto &c = reg.uiCanvases[e];
    out << "CANVAS " << std::quoted(c.name) << " " << (c.isActive ? 1 : 0) << "\n";
}
void SceneManager::LoadCanvas(Entity e, std::istream &in, Registry &reg) {
    UICanvasComponent c;
    int active;
    in >> std::quoted(c.name) >> active;
    c.isActive = (active == 1);
    reg.AddComponent(e, c);
}

// --- UI ---
void SceneManager::SaveUI(Entity e, Registry &reg, std::ostream &out) {
    auto &ui = reg.uiComponents[e];
    out << "UI " << (int)ui.type << " " << std::quoted(ui.name) << " " 
        << std::quoted(ui.text) << " " << (int)ui.anchor << " "
        << ui.offset.x << " " << ui.offset.y << " " << ui.size.x << " "
        << ui.size.y << " " << (int)ui.color.r << " " << (int)ui.color.g
        << " " << (int)ui.color.b << " " << (int)ui.color.a << " " << ui.parentCanvas << "\n";
}
void SceneManager::LoadUI(Entity e, std::istream &in, Registry &reg) {
    UIComponent ui;
    int type, anchor, r, g, b, a;
    in >> type >> std::quoted(ui.name) >> std::quoted(ui.text) >> anchor 
       >> ui.offset.x >> ui.offset.y >> ui.size.x >> ui.size.y >> r >> g >> b >> a >> ui.parentCanvas;
    ui.type = (UIType)type;
    ui.anchor = (UIAnchor)anchor;
    ui.color = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
    reg.AddComponent(e, ui);
}

// --- SCRIPTS ---
void SceneManager::SaveScripts(Entity e, Registry &reg, std::ostream &out) {
    auto &sc = reg.scripts[e];
    out << "SCRIPTS " << sc.instances.size() << "\n";
    for (const auto &inst : sc.instances) {
        out << "INSTANCE " << std::quoted(inst.path) << " " << inst.properties.size() << "\n";
        for (const auto &p : inst.properties) {
            out << "PROPERTY " << (int)p.type << " " << std::quoted(p.name) << " ";
            switch (p.type) {
                case PROP_FLOAT: out << p.floatValue << "\n"; break;
                case PROP_INT: out << p.intValue << "\n"; break;
                case PROP_BOOL: out << (p.boolValue ? 1 : 0) << "\n"; break;
                case PROP_STRING: out << std::quoted(p.stringValue) << "\n"; break;
                case PROP_VECTOR2: out << p.vectorValue.x << " " << p.vectorValue.y << "\n"; break;
                case PROP_COLOR: out << (int)p.colorValue.r << " " << (int)p.colorValue.g << " " << (int)p.colorValue.b << " " << (int)p.colorValue.a << "\n"; break;
            }
        }
    }
}
void SceneManager::LoadScripts(Entity e, std::istream &in_line, std::istream &in_file, Registry &reg) {
    int instCount;
    in_line >> instCount;
    ScriptComponent sc;
    for (int j = 0; j < instCount; j++) {
        std::string line;
        while (std::getline(in_file, line) && line.empty());
        
        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;
        if (cmd == "INSTANCE") {
            ScriptInstanceData inst;
            int propCount;
            ss >> std::quoted(inst.path) >> propCount;
            for (int k = 0; k < propCount; k++) {
                if (!std::getline(in_file, line)) break;
                std::stringstream ssProp(line);
                ssProp >> cmd; 
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
            sc.instances.push_back(inst);
        }
    }
    reg.AddComponent(e, sc);
}

// --- ANIMATION ---
void SceneManager::SaveAnimation(Entity e, Registry &reg, std::ostream &out) {
    auto &sa = reg.spriteAnimations[e];
    out << "ANIMATION " << sa.columns << " " << sa.rows << " " << std::quoted(sa.currentState) << " " << sa.states.size() << "\n";
    for (const auto &pair : sa.states) {
        const auto &s = pair.second;
        out << "STATE " << std::quoted(s.name) << " " << s.startFrame << " " << s.startRow << " " << s.endFrame << " " << s.endRow << " " << s.frameDuration << " " << (s.loop ? 1 : 0) << "\n";
    }
}
void SceneManager::LoadAnimation(Entity e, std::istream &in_line, std::istream &in_file, Registry &reg) {
    SpriteAnimationComponent sa;
    int stateCount;
    in_line >> sa.columns >> sa.rows >> std::quoted(sa.currentState) >> stateCount;
    for (int j = 0; j < stateCount; j++) {
        std::string line;
        while (std::getline(in_file, line) && line.empty());
        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;
        if (cmd == "STATE") {
            AnimationState s;
            int loop;
            ss >> std::quoted(s.name) >> s.startFrame >> s.startRow >> s.endFrame >> s.endRow >> s.frameDuration >> loop;
            s.loop = (loop == 1);
            sa.states[s.name] = s;
        }
    }
    reg.AddComponent(e, sa);
}

// --- PHYSICS ---
void SceneManager::SavePhysics(Entity e, Registry &reg, std::ostream &out) {
    auto &ph = reg.rigidPhysicsComponents[e];
    out << "PHYSICS " << ph.mass << " " << ph.friction << " " << ph.restitution << " " << ph.gravityScale << " " << (ph.isStatic ? 1 : 0) << " " << (ph.affectedByGravity ? 1 : 0) << "\n";
}
void SceneManager::LoadPhysics(Entity e, std::istream &in, Registry &reg) {
    RigidPhysicsComponent ph;
    int isStatic, affectedByGravity;
    in >> ph.mass >> ph.friction >> ph.restitution >> ph.gravityScale >> isStatic >> affectedByGravity;
    ph.isStatic = (isStatic == 1);
    ph.affectedByGravity = (affectedByGravity == 1);
    reg.AddComponent(e, ph);
}

// --- CIRCLE COLLIDER ---
void SceneManager::SaveCircleCollider(Entity e, Registry &reg, std::ostream &out) {
    auto &cc = reg.circleColliders[e];
    out << "CIRCLE_COLLIDER " << cc.offset.x << " " << cc.offset.y << " " << cc.radius << " " << (cc.debugDraw ? 1 : 0) << "\n";
}
void SceneManager::LoadCircleCollider(Entity e, std::istream &in, Registry &reg) {
    CircleColliderComponent cc;
    int debug;
    in >> cc.offset.x >> cc.offset.y >> cc.radius >> debug;
    cc.debugDraw = (debug == 1);
    reg.AddComponent(e, cc);
}

// --- BOX COLLIDER ---
void SceneManager::SaveBoxCollider(Entity e, Registry &reg, std::ostream &out) {
    auto &bc = reg.boxColliders[e];
    out << "BOX_COLLIDER " << bc.offset.x << " " << bc.offset.y << " " << bc.size.x << " " << bc.size.y << " " << (bc.debugDraw ? 1 : 0) << "\n";
}
void SceneManager::LoadBoxCollider(Entity e, std::istream &in, Registry &reg) {
    BoxColliderComponent bc;
    int debug;
    in >> bc.offset.x >> bc.offset.y >> bc.size.x >> bc.size.y >> debug;
    bc.debugDraw = (debug == 1);
    reg.AddComponent(e, bc);
}

// --- MATERIAL ---
void SceneManager::SaveMaterial(Entity e, Registry &reg, std::ostream &out) {
    auto &mat = reg.materials[e];
    out << "MATERIAL " << std::quoted(mat.shaderName) << " " << (int)mat.color.r << " " << (int)mat.color.g << " " << (int)mat.color.b << " " << (int)mat.color.a << "\n";
}
void SceneManager::LoadMaterial(Entity e, std::istream &in, Registry &reg, Engine *engine) {
    MaterialComponent mat;
    int r, g, b, a;
    in >> std::quoted(mat.shaderName) >> r >> g >> b >> a;
    mat.color = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
    reg.AddComponent(e, mat);
}

// --- LIGHT ---
void SceneManager::SaveLight(Entity e, Registry &reg, std::ostream &out) {
    auto &l = reg.lights[e];
    out << "LIGHT " << l.type << " " << (int)l.color.r << " " << (int)l.color.g << " " << (int)l.color.b << " " << (int)l.color.a << " " << l.intensity << " " << l.radius << "\n";
}
void SceneManager::LoadLight(Entity e, std::istream &in, Registry &reg) {
    LightComponent l;
    int r, g, b, a;
    in >> l.type >> r >> g >> b >> a >> l.intensity >> l.radius;
    l.color = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
    reg.AddComponent(e, l);
}

// --- CAMERA ---
void SceneManager::SaveCamera(Entity e, Registry &reg, std::ostream &out) {
    auto &cam = reg.cameras[e];
    out << "CAMERA " << cam.zoom << " " << cam.offset.x << " " << cam.offset.y << " " << cam.target.x << " " << cam.target.y << " " << cam.rotation << " " << (cam.isPrimary ? 1 : 0) << "\n";
}
void SceneManager::LoadCamera(Entity e, std::istream &in, Registry &reg) {
    CameraComponent cam;
    int primary;
    in >> cam.zoom >> cam.offset.x >> cam.offset.y >> cam.target.x >> cam.target.y >> cam.rotation >> primary;
    cam.isPrimary = (primary == 1);
    reg.AddComponent(e, cam);
}

// --- TILEMAP ---
void SceneManager::SaveTilemap(Entity e, Registry &reg, std::ostream &out) {
    auto &map = reg.tilemaps[e];
    out << "TILEMAP " << map.width << " " << map.height << " " << map.tileSize << " " << std::quoted(map.tileSetPath) << " " << map.tiles.size() << "\n";
    for (const auto &tile : map.tiles) {
        out << "TILE " << tile.index << " " << (tile.flipX ? 1 : 0) << " " << (tile.flipY ? 1 : 0) << " " 
            << (int)tile.tint.r << " " << (int)tile.tint.g << " " << (int)tile.tint.b << " " << (int)tile.tint.a << "\n";
    }
}

void SceneManager::LoadTilemap(Entity e, std::istream &in_line, std::istream &in_file, Registry &reg, Engine *engine) {
    TilemapComponent map;
    int tileCount;
    in_line >> map.width >> map.height >> map.tileSize >> std::quoted(map.tileSetPath) >> tileCount;
    
    for (int i = 0; i < tileCount; i++) {
        std::string line;
        while (std::getline(in_file, line) && line.empty());
        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;
        if (cmd == "TILE") {
            Tile tile;
            int fx, fy, r, g, b, a;
            ss >> tile.index >> fx >> fy >> r >> g >> b >> a;
            tile.flipX = (fx == 1);
            tile.flipY = (fy == 1);
            tile.tint = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
            map.tiles.push_back(tile);
        }
    }

    // Load TileSet into ResourceManager if path provided
    if (!map.tileSetPath.empty() && engine) {
        // Here we'd load the .tileset asset. 
        // For now, let's assume the Editor handles creating/loading the TileSet resource.
    }

    reg.AddComponent(e, map);
}
