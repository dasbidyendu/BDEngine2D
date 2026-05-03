#include "InputManager.h"
#include "Utils/Logger.h"

bool InputManager::keyPressed(std::string key) {
    if (keyCodes.find(key) != keyCodes.end()) {
        KeyboardKey code = keyCodes[key];
		return IsKeyPressed(code);
    }
    else {
		Logger::AddLog(LOG_LEVEL_WARNING, "InputManager: Key '%s' not found in keyCodes map!", key.c_str());
		return false; // Key not found
    }
}
bool InputManager::keyReleased(std::string key) {
    if (keyCodes.find(key) != keyCodes.end()) {
        KeyboardKey code = keyCodes[key];
        return IsKeyReleased(code);
    }
    else {
        Logger::AddLog(LOG_LEVEL_WARNING, "InputManager: Key '%s' not found in keyCodes map!", key.c_str());
        return false; // Key not found
    }
}
bool InputManager::keyDown(std::string key) {
    if (keyCodes.find(key) != keyCodes.end()) {
        KeyboardKey code = keyCodes[key];
        return IsKeyDown(code);
    }
    else {
        Logger::AddLog(LOG_LEVEL_WARNING, "InputManager: Key '%s' not found in keyCodes map!", key.c_str());
        return false; // Key not found
    }
}
bool InputManager::keyUp(std::string key) {
    if (keyCodes.find(key) != keyCodes.end()) {
        KeyboardKey code = keyCodes[key];
        return IsKeyUp(code);
    }
    else {
        Logger::AddLog(LOG_LEVEL_WARNING, "InputManager: Key '%s' not found in keyCodes map!", key.c_str());
        return false; // Key not found
    }
}
bool InputManager::keyPressedRepeat(std::string key) {
    if (keyCodes.find(key) != keyCodes.end()) {
        KeyboardKey code = keyCodes[key];
        return IsKeyPressedRepeat(code);
    }
    else {
        Logger::AddLog(LOG_LEVEL_WARNING, "InputManager: Key '%s' not found in keyCodes map!", key.c_str());
        return false; // Key not found
    }
}

bool InputManager::mouseDown(std::string button) {
    if (mouseButtonCodes.find(button) != mouseButtonCodes.end()) {
        MouseButton code = mouseButtonCodes[button];
        return IsMouseButtonDown(code);
    }
    else {
        Logger::AddLog(LOG_LEVEL_WARNING, "InputManager: Mouse button '%s' not found in mouseButtonCodes map!", button.c_str());
        return false; // Mouse button not found
    }
}

bool InputManager::mouseUp(std::string button) {
    if (mouseButtonCodes.find(button) != mouseButtonCodes.end()) {
        MouseButton code = mouseButtonCodes[button];
        return IsMouseButtonUp(code);
    }
    else {
        Logger::AddLog(LOG_LEVEL_WARNING, "InputManager: Mouse button '%s' not found in mouseButtonCodes map!", button.c_str());
        return false; // Mouse button not found
    }
}

bool InputManager::mousePressed(std::string button) {
    if (mouseButtonCodes.find(button) != mouseButtonCodes.end()) {
        MouseButton code = mouseButtonCodes[button];
        return IsMouseButtonPressed(code);
    }
    else {
        Logger::AddLog(LOG_LEVEL_WARNING, "InputManager: Mouse button '%s' not found in mouseButtonCodes map!", button.c_str());
        return false; // Mouse button not found
    }
}
bool InputManager::mouseReleased(std::string button) {
    if (mouseButtonCodes.find(button) != mouseButtonCodes.end()) {
        MouseButton code = mouseButtonCodes[button];
        return IsMouseButtonReleased(code);
    }
    else {
        Logger::AddLog(LOG_LEVEL_WARNING, "InputManager: Mouse button '%s' not found in mouseButtonCodes map!", button.c_str());
        return false; // Mouse button not found
    }
}

InputManager::InputManager() {
    // Alphanumeric keys (A-Z already started)
    keyCodes["A"] = KEY_A;
    keyCodes["B"] = KEY_B;
    keyCodes["C"] = KEY_C;
    keyCodes["D"] = KEY_D;
    keyCodes["E"] = KEY_E;
    keyCodes["F"] = KEY_F;
    keyCodes["G"] = KEY_G;
    keyCodes["H"] = KEY_H;
    keyCodes["I"] = KEY_I;
    keyCodes["J"] = KEY_J;
    keyCodes["K"] = KEY_K;
    keyCodes["L"] = KEY_L;
    keyCodes["M"] = KEY_M;
    keyCodes["N"] = KEY_N;
    keyCodes["O"] = KEY_O;
    keyCodes["P"] = KEY_P;
    keyCodes["Q"] = KEY_Q;
    keyCodes["R"] = KEY_R;
    keyCodes["S"] = KEY_S;
    keyCodes["T"] = KEY_T;
    keyCodes["U"] = KEY_U;
    keyCodes["V"] = KEY_V;
    keyCodes["W"] = KEY_W;
    keyCodes["X"] = KEY_X;
    keyCodes["Y"] = KEY_Y;
    keyCodes["Z"] = KEY_Z;

    // Numbers
    keyCodes["0"] = KEY_ZERO;
    keyCodes["1"] = KEY_ONE;
    keyCodes["2"] = KEY_TWO;
    keyCodes["3"] = KEY_THREE;
    keyCodes["4"] = KEY_FOUR;
    keyCodes["5"] = KEY_FIVE;
    keyCodes["6"] = KEY_SIX;
    keyCodes["7"] = KEY_SEVEN;
    keyCodes["8"] = KEY_EIGHT;
    keyCodes["9"] = KEY_NINE;

    // Punctuation / Symbols
    keyCodes[" "] = KEY_SPACE;
    keyCodes[";"] = KEY_SEMICOLON;
    keyCodes["="] = KEY_EQUAL;
    keyCodes[","] = KEY_COMMA;
    keyCodes["-"] = KEY_MINUS;
    keyCodes["."] = KEY_PERIOD;
    keyCodes["/"] = KEY_SLASH;
    keyCodes["["] = KEY_LEFT_BRACKET;
    keyCodes["\\"] = KEY_BACKSLASH;
    keyCodes["]"] = KEY_RIGHT_BRACKET;
    keyCodes["`"] = KEY_GRAVE;
    keyCodes["\""] = KEY_APOSTROPHE;

    // Special Keys (assuming your map key type allows strings, otherwise use a separate map)
    // For Lua accessibility, using string names is often easier than magic numbers.
    keyCodes["Escape"] = KEY_ESCAPE;
    keyCodes["Enter"] = KEY_ENTER;
    keyCodes["Tab"] = KEY_TAB;
    keyCodes["Backspace"] = KEY_BACKSPACE;
    keyCodes["Insert"] = KEY_INSERT;
    keyCodes["Delete"] = KEY_DELETE;
    keyCodes["Up"] = KEY_UP;
    keyCodes["Down"] = KEY_DOWN;
    keyCodes["Left"] = KEY_LEFT;
    keyCodes["Right"] = KEY_RIGHT;
	keyCodes["Space"] = KEY_SPACE;

    // Modifiers
    keyCodes["LShift"] = KEY_LEFT_SHIFT;
    keyCodes["RShift"] = KEY_RIGHT_SHIFT;
    keyCodes["LCtrl"] = KEY_LEFT_CONTROL;
    keyCodes["RCtrl"] = KEY_RIGHT_CONTROL;
    keyCodes["LAlt"] = KEY_LEFT_ALT;
    keyCodes["RAlt"] = KEY_RIGHT_ALT;

    // Function Keys
    keyCodes["F1"] = KEY_F1;
    keyCodes["F2"] = KEY_F2;
    keyCodes["F3"] = KEY_F3;
    keyCodes["F4"] = KEY_F4;
    keyCodes["F5"] = KEY_F5;
    keyCodes["F6"] = KEY_F6;
    keyCodes["F7"] = KEY_F7;
    keyCodes["F8"] = KEY_F8;
    keyCodes["F9"] = KEY_F9;
    keyCodes["F10"] = KEY_F10;
    keyCodes["F11"] = KEY_F11;
    keyCodes["F12"] = KEY_F12;

    //Mouse Buttons
    mouseButtonCodes["Left"] = MOUSE_LEFT_BUTTON;
    mouseButtonCodes["Right"] = MOUSE_RIGHT_BUTTON;
    mouseButtonCodes["Middle"] = MOUSE_MIDDLE_BUTTON;
    mouseButtonCodes["Side"] = MOUSE_BUTTON_SIDE;
    mouseButtonCodes["Extra"] = MOUSE_BUTTON_EXTRA;
    mouseButtonCodes["Forward"] = MOUSE_BUTTON_FORWARD;
    mouseButtonCodes["Back"] = MOUSE_BUTTON_BACK;
}