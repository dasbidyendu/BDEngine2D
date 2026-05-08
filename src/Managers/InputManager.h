#pragma once

#include "raylib.h"
#include <unordered_map>
#include <string>


class InputManager {
public:
	InputManager();
	 bool keyPressed(std::string key);
	 bool keyReleased(std::string key);
	 bool keyDown(std::string key);
	 bool keyUp(std::string key);
	 bool keyPressedRepeat(std::string key);
	 bool mouseDown(std::string button);
	 bool mouseUp(std::string button);
	 bool mousePressed(std::string button);
	 bool mouseReleased(std::string button);
private:
	std::unordered_map<std::string, KeyboardKey>keyCodes;
	std::unordered_map<std::string, MouseButton>mouseButtonCodes;
};