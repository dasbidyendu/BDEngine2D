#pragma once
#include "raylib.h"
#include <string>
#include "Managers/ResourceManager.h"
#include "ECS/Registry.h"
#include "ECS/Systems.h"
#include <fstream>
#include <sstream>


class Engine {
public:
	Engine(int width, int height, const std::string& title);

	~Engine();

	void InitGame();

	void Run();

	void ApplyTheme(const std::string& themeName);

	void SaveConfig();
	void LoadConfig();

	bool IsMouseOverUI = false;

	std::vector<std::string> themeFiles;
private:
	void Update();

	void Render();

	int screenWidth;
	int screenHeight;
	const std::string windowTitle;
	bool isRunning;
	int selectedAssetIndex = -1;
	bool showGrid = true;
	int gridSize = 32;
	bool isEditorMode = true;

	std::vector<AssetEntry> editorAssets;

	Color gridColor = { 200,200,200,40 };

	ResourceManager assets;

	Registry registry;

	EngineStats stats;

	Camera2D camera = { 0 };
	float zoomSensitivity = 0.1f;

	Entity selectedEntity = -1;

	struct EngineTheme {
		std::string name;
		Color background;
		Color panelBG;
		Color gridColor;
		Color accentColor;
		int fontSize;

		unsigned int borderId;
		unsigned int textId;
	} currentTheme;

	bool showSettings = false;
	int settingsActiveTab = 0;

	std::string lastThemePath = "assets/themes/dark-gold.txt";
	void ScanThemes();
};


