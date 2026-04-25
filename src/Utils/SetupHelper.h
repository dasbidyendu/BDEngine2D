#pragma once

#include "imgui.h"
#include "Logger.h"
#include <fstream>
#include <json.hpp>

using json = nlohmann::json;

namespace SetupHelper {
	
	static inline void InitImguiStyle(ImGuiIO& io) {
        std::ifstream themeFile("engine-configs/editor-theme-config.json");
        if (!themeFile.is_open()) {
            Logger::AddLog(LOG_LEVEL_ERROR, "Failed to open theme configuration file: editor-configs/editor-theme-config.json");
            return;
        }
        json theme;
        themeFile >> theme;
        ImGuiStyle& style = ImGui::GetStyle();

        // 1. Layout & Geometry (The "Cozy" Feel)
        style.WindowRounding = theme["style"]["rounding"]["window"];
        style.ChildRounding = theme["style"]["rounding"]["child"];
        style.FrameRounding = theme["style"]["rounding"]["frame"];
        style.GrabRounding = theme["style"]["rounding"]["grab"];
        style.PopupRounding = theme["style"]["rounding"]["popup"];
        // Padding & Spacing
        style.WindowPadding = ImVec2(theme["style"]["padding"]["window"][0], theme["style"]["padding"]["window"][1]);
        style.FramePadding = ImVec2(theme["style"]["padding"]["frame"][0], theme["style"]["padding"]["frame"][1]);
        style.ItemSpacing = ImVec2(theme["style"]["spacing"]["item"][0], theme["style"]["spacing"]["item"][1]);
        style.ItemInnerSpacing = ImVec2(theme["style"]["spacing"]["item_inner"][0], theme["style"]["spacing"]["item_inner"][1]);

        // Misc
        style.ScrollbarSize = theme["style"]["misc"]["scrollbar_size"];
        style.WindowBorderSize = theme["style"]["misc"]["window_border"];
        style.ChildBorderSize = theme["style"]["misc"]["child_border"];

        // 2. Apply Colors
        auto& colors = style.Colors;
        auto setCol = [&](ImGuiCol_ idx, const std::string& key) {
            auto c = theme["colors"][key];
            colors[idx] = ImVec4(c[0], c[1], c[2], c[3]);
            };

        setCol(ImGuiCol_WindowBg, "window_bg");
        setCol(ImGuiCol_ChildBg, "child_bg");
        setCol(ImGuiCol_PopupBg, "popup_bg");
        setCol(ImGuiCol_Header, "header");
        setCol(ImGuiCol_HeaderHovered, "header_hovered");
        setCol(ImGuiCol_HeaderActive, "header_active");
        setCol(ImGuiCol_Button, "button");
        setCol(ImGuiCol_ButtonHovered, "button_hovered");
        setCol(ImGuiCol_ButtonActive, "button_active");
        setCol(ImGuiCol_FrameBg, "frame_bg");
        setCol(ImGuiCol_FrameBgHovered, "frame_bg_hovered");
        setCol(ImGuiCol_FrameBgActive, "frame_bg_active");
        setCol(ImGuiCol_Tab, "tab");
        setCol(ImGuiCol_TabHovered, "tab_hovered");
        setCol(ImGuiCol_TabActive, "tab_active");
        setCol(ImGuiCol_TitleBgActive, "title_bg_active");
        setCol(ImGuiCol_TitleBg, "title_bg");
        setCol(ImGuiCol_Text, "text");

        std::string fontPath = theme.value("font-path", "");
        if (!fontPath.empty()) {
            ImGuiIO& io = ImGui::GetIO();
            ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f);
            if (font) {
                io.FontDefault = font;
                io.Fonts->Build();
            }
        }
		themeFile.close();
	}

}
