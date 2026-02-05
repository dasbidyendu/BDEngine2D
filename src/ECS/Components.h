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

enum UIType { UI_BUTTON, UI_TEXT, UI_IMAGE, UI_PANEL, UI_CHECKBOX, UI_SLIDER, UI_LABEL };

struct UICanvasComponent {
    std::string name = "New Canvas";
    bool isActive = true;
};

struct UIComponent {
    UIType type = UI_BUTTON;
    std::string name = "Widget";

    Entity parentCanvas = -1;

    UIAnchor anchor = ANCHOR_TOP_LEFT;
    Vector2 offset = { 100, 100 };
    Vector2 size = { 150, 40 };

    Color color = DARKGRAY;
	std::string text = "Text";
    int fontSize = 20;

    // For UI_IMAGE
    std::string texturePath = "";
    Texture2D texture = { 0 };

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
};