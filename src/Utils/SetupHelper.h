#pragma once

#include "imgui.h"
#include "Logger.h"

namespace SetupHelper {
	
	static inline void InitImguiStyle(ImGuiIO& io) {
        ImGuiStyle& style = ImGui::GetStyle();

        // 1. Layout & Geometry (The "Cozy" Feel)
        style.WindowRounding = 10.0f;           // Soft corners
        style.ChildRounding = 6.0f;
        style.FrameRounding = 6.0f;
        style.GrabRounding = 6.0f;
        style.PopupRounding = 6.0f;

        style.WindowPadding = ImVec2(10, 10);
        style.FramePadding = ImVec2(8, 6);      // More breathing room
        style.ItemSpacing = ImVec2(8, 8);
        style.ItemInnerSpacing = ImVec2(4, 4);
        style.ScrollbarSize = 12.0f;
        style.WindowBorderSize = 0.0f;          // Remove hard borders
        style.ChildBorderSize = 0.0f;

        // 2. The Color Palette (Deep Warm-Dark Theme)
        // Using a soft "Midnight Charcoal" base with "Muted Amber" accents
        auto& colors = style.Colors;

        // Backgrounds
        colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);

        // Headers & Accents (The "Muted Amber" personality)
        colors[ImGuiCol_Header] = ImVec4(0.25f, 0.23f, 0.20f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.32f, 0.28f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.40f, 0.35f, 1.00f);

        // Buttons
        colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.29f, 0.27f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.38f, 0.35f, 1.00f);

        // Frame background (inputs, checkboxes)
        colors[ImGuiCol_FrameBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.23f, 0.20f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.23f, 0.20f, 1.00f);

        // Title bar
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);

        // Text
        colors[ImGuiCol_Text] = ImVec4(0.85f, 0.83f, 0.80f, 1.00f);

        std::string fontPath = "assets/fonts/Poppins-SemiBold.ttf";

        ImFont* myCustomFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f);

        if (myCustomFont == nullptr) {
            Logger::AddLog(LOG_LEVEL_ERROR, "Failed to load font: %s", fontPath.c_str());
        }

		io.FontDefault = myCustomFont; 
	}

}
