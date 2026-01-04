#pragma once
#include "../ECS/Registry.h"
#include "raylib.h"
#include "raymath.h"

namespace UISystem {

    // Helper to calculate exact screen position based on anchor
    inline Rectangle GetRect(const UIComponent& ui) {
        float sw = (float)GetScreenWidth();
        float sh = (float)GetScreenHeight();
        Vector2 pos = { 0, 0 };

        // 1. Calculate the Reference Point (The Anchor on Screen)
        switch (ui.anchor) {
        case ANCHOR_TOP_LEFT:    pos = { 0, 0 }; break;
        case ANCHOR_TOP_RIGHT:   pos = { sw, 0 }; break;
        case ANCHOR_CENTER:      pos = { sw * 0.5f, sh * 0.5f }; break;
        case ANCHOR_BOTTOM_LEFT: pos = { 0, sh }; break;
        }

        // 2. Adjust for Element Size (Pivot Adjustment)
        // By default, we assume the pivot is consistent with the anchor
        Vector2 alignment = { 0, 0 };
        if (ui.anchor == ANCHOR_TOP_RIGHT)   alignment = { -ui.size.x, 0 };
        if (ui.anchor == ANCHOR_CENTER)      alignment = { -ui.size.x * 0.5f, -ui.size.y * 0.5f };
        if (ui.anchor == ANCHOR_BOTTOM_LEFT) alignment = { 0, -ui.size.y };

        // 3. Final Position = Anchor + Alignment + User Offset
        return {
            pos.x + alignment.x + ui.offset.x,
            pos.y + alignment.y + ui.offset.y,
            ui.size.x,
            ui.size.y
        };
    }

    inline void Draw(Registry& reg) {
        for (Entity i = 0; i < MAX_ENTITIES; i++) {
            if (reg.HasComponent(i, COMP_UI)) {
                auto& ui = reg.uiComponents[i];
                Rectangle r = GetRect(ui);

                // Draw Panel
                DrawRectangleRec(r, ui.color);
                DrawRectangleLinesEx(r, 2, Fade(BLACK, 0.3f));

                // Draw Text (Centered)
                int textW = MeasureText(ui.text.c_str(), ui.fontSize);
                DrawText(ui.text.c_str(),
                    (int)(r.x + (r.width / 2) - (textW / 2)),
                    (int)(r.y + (r.height / 2) - (ui.fontSize / 2)),
                    ui.fontSize, WHITE);
            }
        }
    }
}