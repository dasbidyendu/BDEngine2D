#pragma once
#include "raylib.h"
#include <functional>
#include <vector>
#include <string>
#include "Entity.h"


struct TransformComponent {
    Vector2 position;
    Vector2 scale;
    float rotation;
};

struct SpriteComponent {
	std::string texturePath;
    Texture2D texture;
    Color tint = WHITE;
    Vector2 anchor = { 0.0f, 0.0f };
    bool flipX = false;
};

struct VelocityComponent {
    Vector2 speed;
};

struct InputComponent {
    bool up, down, left, right;
};


struct ScriptComponent {
    std::vector<std::string> scriptPaths;
    bool isInitialized = false;
};

enum UIAnchor { ANCHOR_TOP_LEFT, ANCHOR_TOP_RIGHT, ANCHOR_CENTER, ANCHOR_BOTTOM_LEFT };

struct UIComponent {
    std::string text = "Button";
    UIAnchor anchor = ANCHOR_TOP_LEFT;
    Vector2 offset = { 100, 100 };
    Vector2 size = { 150, 40 };
    Color color = DARKGRAY;
    int fontSize = 20; // Added this for better text control

    // Editor State (Not saved, used for dragging)
    bool isDragging = false;

    // Runtime Logic
    bool isPressed = false;
    std::function<void()> onClick;
};