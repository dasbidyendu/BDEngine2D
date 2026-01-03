#pragma once
#include "raylib.h"

struct DebugStats {
    bool visible = false;
    int frameCount = 0;
    float fpsHistory[60] = { 0 };
    int historyIndex = 0;
    
    // For advanced profiling
    float logicTime = 0.0f;
    float scriptTime = 0.0f;
    float renderTime = 0.0f;

    void UpdateHistory() {
        fpsHistory[historyIndex] = (float)GetFPS();
        historyIndex = (historyIndex + 1) % 60;
    }
    
    void Toggle() { visible = !visible; }
};
