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

    // In ECS/UISystem.h
    inline void Draw(Registry& reg) {
        for (Entity i = 0; i < MAX_ENTITIES; i++) {
            if (!reg.HasComponent(i, COMP_UI)) continue;

            auto& ui = reg.uiComponents[i];
            Rectangle r = GetRect(ui);

            switch (ui.type) {
            case UI_PANEL:
                DrawRectangleRec(r, ui.color);
                DrawRectangleLinesEx(r, 1, Fade(BLACK, 0.5f));
                break;
            case UI_BUTTON:
                DrawRectangleRec(r, ui.color);
                DrawRectangleLinesEx(r, 2, WHITE);
                DrawText(ui.text.c_str(), r.x + 10, r.y + (r.height / 2 - ui.fontSize / 2), ui.fontSize, WHITE);
                break;
            case UI_LABEL:
                DrawText(ui.text.c_str(), r.x, r.y, ui.fontSize, ui.color);
                break;
            case UI_IMAGE:
                if (ui.texture.id != 0) {
                    DrawTexturePro(ui.texture,
                        { 0, 0, (float)ui.texture.width, (float)ui.texture.height },
                        r, { 0,0 }, 0, ui.color);
                }
                else {
                    DrawRectangleLinesEx(r, 1, RED);
                    DrawText("MISSING IMG", r.x + 5, r.y + 5, 10, RED);
                }
                break;
            }
        }
    }
}