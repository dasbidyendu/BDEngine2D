#pragma once
#include "raylib.h"
#include <functional>

struct TransformComponent {
	Vector2 position;
	Vector2 scale;
	float rotation;
};

struct SpriteComponent {
	Texture2D texture;
	Color tint;
};

struct VelocityComponent {
	Vector2 speed;
};

struct InputComponent {
	bool up, down, left, right;
};

enum UIAnchor { ANCHOR_TOP_LEFT, ANCHOR_TOP_RIGHT, ANCHOR_CENTER, ANCHOR_BOTTOM_LEFT };

struct UIComponent {
	std::string text;
	UIAnchor anchor;
	Vector2 offset;    
	Vector2 size;
	Color color;
	bool isPressed;    

	std::function<void()> onClick;
};