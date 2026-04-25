#pragma once
#include "raylib.h"
#include <unordered_map>
#include <string>

struct Hotkey {
    int key;
    bool ctrl;
    bool shift;
    bool alt;

    bool IsPressed() const {
        bool mods = true;
        if (ctrl) mods &= (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL));
        if (shift) mods &= (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
        if (alt) mods &= (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT));
        
        return mods && IsKeyPressed(key);
    }
};

class HotkeyManager {
public:
    static HotkeyManager& Get() {
        static HotkeyManager instance;
        return instance;
    }

    void Bind(const std::string& action, int key, bool ctrl = false, bool shift = false, bool alt = false) {
        hotkeys[action] = { key, ctrl, shift, alt };
    }

    bool IsPressed(const std::string& action) {
        if (hotkeys.find(action) != hotkeys.end()) {
            return hotkeys[action].IsPressed();
        }
        return false;
    }

    std::unordered_map<std::string, Hotkey>& GetHotkeys() { return hotkeys; }

private:
    HotkeyManager() {
        // Default bindings
        Bind("Focus", KEY_F);
        Bind("Delete", KEY_DELETE);
        Bind("GizmoTranslate", KEY_W);
        Bind("GizmoRotate", KEY_E);
        Bind("GizmoScale", KEY_R);
        Bind("SaveScene", KEY_S, true); // Ctrl + S
    }
    std::unordered_map<std::string, Hotkey> hotkeys;
};
