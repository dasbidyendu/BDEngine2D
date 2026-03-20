#pragma once
#include "Entity.h"
#include "raylib.h"
#include <functional>
#include <string>
#include <vector>

struct NameComponent {
  std::string name = "Entity";
};

struct alignas(16) TransformComponent {
  Vector2 position;
  Vector2 scale;
  float rotation;
  float padding;
};

struct SpriteComponent {
  std::string texturePath;
  Texture2D texture;
  Color tint = WHITE;
  Vector2 anchor = {0.0f, 0.0f};
  bool flipX = false;
};

struct alignas(16) VelocityComponent {
  Vector2 speed;
  float pad[2];
};

struct InputComponent {
  bool up, down, left, right;
};

enum ScriptPropertyType {
    PROP_FLOAT,
    PROP_INT,
    PROP_BOOL,
    PROP_STRING,
    PROP_VECTOR2,
    PROP_COLOR
};

struct ScriptProperty {
    ScriptPropertyType type;
    std::string name;
    float floatValue = 0.0f;
    int intValue = 0;
    bool boolValue = false;
    std::string stringValue = "";
    Vector2 vectorValue = {0, 0};
    Color colorValue = WHITE;
};

struct ScriptInstanceData {
    std::string path;
    std::vector<ScriptProperty> properties;
    bool started = false;
};

struct ScriptComponent {
  std::vector<ScriptInstanceData> instances;
  std::vector<std::string> scriptPaths; // Legacy, but keeping for compatibility if needed. Actually let's just use instances.
  bool isInitialized = false;
};

enum UIAnchor {
  ANCHOR_TOP_LEFT,
  ANCHOR_TOP_RIGHT,
  ANCHOR_CENTER,
  ANCHOR_BOTTOM_LEFT
};

enum UIType {
  UI_BUTTON,
  UI_TEXT,
  UI_IMAGE,
  UI_PANEL,
  UI_CHECKBOX,
  UI_SLIDER,
  UI_LABEL
};

struct UICanvasComponent {
  std::string name = "New Canvas";
  bool isActive = true;
};

struct UIComponent {
  UIType type = UI_BUTTON;
  std::string name = "Widget";

  Entity parentCanvas = -1;

  UIAnchor anchor = ANCHOR_TOP_LEFT;
  Vector2 offset = {100, 100};
  Vector2 size = {150, 40};

  Color color = DARKGRAY;
  std::string text = "Text";
  int fontSize = 20;

  // For UI_IMAGE
  std::string texturePath = "";
  Texture2D texture = {0};

  // Editor/Runtime State
  bool isDragging = false;
  bool isPressed = false;
  std::function<void()> onClick;
};

struct SpriteAnimationComponent {
  int frameCount = 1;
  int rowCount = 1;
  int currentFrame = 0;
  int currentRow = 0;
  float frameDuration = 0.1f;
  float elapsedTime = 0.0f;
  bool isPlaying = true;
  bool loop = true;
};

struct alignas(16) RigidPhysicsComponent {
  float mass = 1.0f;
  float friction = 0.1f;
  float restitution = 0.5f;
  float gravityScale = 100.0f;
  bool isStatic = false;
  bool affectedByGravity = true;
};

struct CircleColliderComponent {
  Vector2 offset = {0, 0};
  float radius = 16.0f;
  bool isColliding = false;
  bool debugDraw = true;
};

struct BoxColliderComponent {
  Vector2 size = {32, 32};
  Vector2 offset = {0, 0};
  bool isColliding = false;
  bool debugDraw = true;
};

struct MaterialComponent {
  Shader shader = {0};
  std::string shaderName = "";
  Color color = WHITE;
  Texture2D texture = {0};
  // We can add arbitrary float/vec properties here in the future via a map if needed, 
  // but for now we'll stick to a primary color and texture to keep it lightweight.
};

struct LightComponent {
  int type = 0; // 0 = Point, 1 = Directional
  Color color = WHITE;
  float intensity = 1.0f;
  float radius = 100.0f;
};

struct CameraComponent {
  float zoom = 1.0f;
  Vector2 offset = {0, 0};
  Vector2 target = {0, 0};
  float rotation = 0.0f;
  bool isPrimary = true;
};