#define _CRT_SECURE_NO_WARNINGS
#include "Engine.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

Engine::Engine(int width, int height, const std::string& title)
	:screenWidth(width),screenHeight(height),windowTitle(title) {
	
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(screenWidth, screenHeight, windowTitle.c_str());
	int monitor = GetCurrentMonitor();
	if (!IsWindowFullscreen()) {
		SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
		ToggleFullscreen();
	}

	SetTargetFPS(0);
	isRunning = true;

	LoadConfig();

	camera.target = { 0.0f, 0.0f };
	camera.offset = { 0.0f, 0.0f };
	camera.rotation = 0.0f;
	camera.zoom = 1.0f;

	GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
	/*assets.LoadTextureAsset("player", "assets/test.png");*/
}

Engine::~Engine() {
	SaveConfig();
	CloseWindow();
}

void Engine::LoadConfig() {
	TraceLog(LOG_INFO, "CONFIG: Loading EditorConfig.ini...");
	std::ifstream file("EditorConfig.ini");

	// Set default resolution before loading
	int targetWidth = 1280;
	int targetHeight = 720;

	if (file.is_open()) {
		std::string line;
		while (std::getline(file, line)) {
			std::istringstream is_line(line);
			std::string key, value;
			if (std::getline(is_line, key, '=') && std::getline(is_line, value)) {
				if (key == "LastTheme") lastThemePath = value;
				if (key == "LastFont") lastFontPath = value;
				if (key == "ShowGrid") showGrid = (value == "true");
				if (key == "GridSize") gridSize = std::stoi(value);
				if (key == "ScreenWidth") targetWidth = std::stoi(value);
				if (key == "ScreenHeight") targetHeight = std::stoi(value);
			}
		}
		file.close();
	}

	// Apply Resolution
	screenWidth = targetWidth;
	screenHeight = targetHeight;
	SetWindowSize(screenWidth, screenHeight);

	// Apply Assets
	ApplyTheme(lastThemePath);
	ScanFonts();
	LoadEngineFont(lastFontPath);
}

void Engine::SaveConfig() {
	std::ofstream file("EditorConfig.ini");
	if (file.is_open()) {
		file << "ScreenWidth=" << GetScreenWidth() << "\n";
		file << "ScreenHeight=" << GetScreenHeight() << "\n";
		file << "LastTheme=" << lastThemePath << "\n";
		file << "LastFont=" << lastFontPath << "\n";
		file << "ShowGrid=" << (showGrid ? "true" : "false") << "\n";
		file << "GridSize=" << gridSize << "\n";
		file.close();
	}
}

void Engine::Run() {
	while (!WindowShouldClose() && isRunning) {
		Update();
		Render();
	}
}

void Engine::InitGame() {
	Entity player = registry.CreateEntity();

	/*registry.AddComponent(player, TransformComponent{ {400, 300}, {1, 1}, 0.0f });
	registry.AddComponent(player, SpriteComponent{ assets.GetTexture("player"), WHITE });
	registry.AddComponent(player, VelocityComponent{ {0, 0} });
	registry.AddComponent(player, InputComponent({ false,false,false,false }));*/

	Entity quitBtn = registry.CreateEntity();
	registry.AddComponent(quitBtn, UIComponent{
		"QUIT",
		ANCHOR_TOP_RIGHT,
		{5, 5},
		{80, 40},
		RED,
		false,
		[this]() { this->isRunning = false; }
		});

	editorAssets = AssetScanner::Scan("assets");
	ScanThemes();

	for (const auto& asset : editorAssets) {
		if (asset.isTexture) {
			assets.LoadTextureAsset(asset.name, asset.path);
		}
	}
}

void Engine::Update() {
	if (IsWindowResized()) {
		screenWidth = GetScreenWidth();
		screenHeight = GetScreenHeight();
	}

	float dt = GetFrameTime();
	Vector2 mousePos = GetMousePosition();

	stats.frameCount++;

	if (IsKeyPressed(KEY_F1)) showSettings = !showSettings;
	if (IsKeyPressed(KEY_TAB)) isEditorMode = !isEditorMode;
	if (isEditorMode && IsKeyPressed(KEY_G)) showGrid = !showGrid;
	

	bool mouseInSidebar = (isEditorMode && mousePos.x < 250);
	bool mouseInInspector = (isEditorMode && mousePos.x > (screenWidth - 250));
	bool mouseInUI = mouseInSidebar || mouseInInspector;

	if (!showSettings) {
		// Game Input & Camera Controls
		if (!mouseInSidebar) {
			InputSystem::Update(registry);
			ControlSystem::Update(registry);

			if (isEditorMode) {
				if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
					Vector2 delta = GetMouseDelta();
					camera.target.x -= delta.x / camera.zoom;
					camera.target.y -= delta.y / camera.zoom;
				}

				float wheel = GetMouseWheelMove();
				if (wheel != 0) {
					Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
					camera.offset = GetMousePosition();
					camera.target = mouseWorldPos;
					camera.zoom += wheel * zoomSensitivity;
					if (camera.zoom < 0.1f) camera.zoom = 0.1f;
					if (camera.zoom > 5.0f) camera.zoom = 5.0f;
				}
			}
		}

		MovementSystem::Update(registry, dt);

		if (isEditorMode) {
			// --- NEW DRAG & DROP PLACEMENT ---
			if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && draggedAssetIndex != -1) {
				// Only place if dropped outside the sidebar
				if (mousePos.x > 250 && !mouseInInspector) {
					Vector2 worldMouse = GetScreenToWorld2D(mousePos, camera);

					Vector2 snappedPos;
					snappedPos.x = floor(worldMouse.x / gridSize) * gridSize;
					snappedPos.y = floor(worldMouse.y / gridSize) * gridSize;

					Entity newEntity = registry.CreateEntity();
					registry.AddComponent(newEntity, TransformComponent{ snappedPos, {1,1}, 0.0f });

					// Use the texture from the manager using the asset name
					registry.AddComponent(newEntity, SpriteComponent{
						assets.GetTexture(editorAssets[draggedAssetIndex].name), WHITE
						});
				}
				draggedAssetIndex = -1; // End drag state
			}

			// --- WORLD SELECTION ---
			if (!mouseInUI && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && draggedAssetIndex == -1) {
				Vector2 worldMouse = GetScreenToWorld2D(mousePos, camera);
				selectedEntity = -1;

				for (Entity i = 0; i < MAX_ENTITIES; i++) {
					if (registry.entityMasks[i].any() && registry.HasComponent(i, COMP_TRANSFORM) && registry.HasComponent(i, COMP_SPRITE)) {
						auto& t = registry.transforms[i];
						auto& s = registry.sprites[i];
						Rectangle bounds = { t.position.x, t.position.y, (float)s.texture.width * t.scale.x, (float)s.texture.height * t.scale.y };

						if (CheckCollisionPointRec(worldMouse, bounds)) {
							selectedEntity = i;
							break;
						}
					}
				}
			}

			// Right Click to Cancel Drag
			if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) draggedAssetIndex = -1;

			// Delete Entity
			if (selectedEntity != -1 && IsKeyPressed(KEY_DELETE)) {
				registry.entityMasks[selectedEntity].reset();
				selectedEntity = -1;
			}
		}
	}

	if (IsKeyPressed(KEY_F3)) {
		if (IsWindowState(FLAG_VSYNC_HINT)) {
			ClearWindowState(FLAG_VSYNC_HINT);
			SetTargetFPS(0);
		}
		else {
			SetWindowState(FLAG_VSYNC_HINT);
		}
	}

	if (IsKeyPressed(KEY_F2)) stats.Toggle();

	if (IsKeyPressed(KEY_F11) || (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER))) {
		ToggleFullscreen();
	}

	if (IsWindowResized() || IsWindowFullscreen()) {
		screenWidth = GetScreenWidth();
		screenHeight = GetScreenHeight();
	}
}

void Engine::Render() {
	BeginDrawing();
	ClearBackground(RAYWHITE);


	//Anything that the camera has to render
	BeginMode2D(camera);
	if (isEditorMode && showGrid) {
		EditorSystem::DrawGrid(gridSize,camera, screenWidth, screenHeight,gridColor);
	}

	if (isEditorMode && selectedEntity != -1) {
		auto& t = registry.transforms[selectedEntity];
		auto& s = registry.sprites[selectedEntity];

		Rectangle outline = {
			t.position.x - 2,
			t.position.y - 2,
			(s.texture.width * t.scale.x) + 4,
			(s.texture.height * t.scale.y) + 4
		};
		DrawRectangleLinesEx(outline, 2.0f / camera.zoom, ORANGE);
	}

	RenderSystem::Draw(registry);

	if (isEditorMode && selectedAssetIndex != -1) {
		// Convert screen mouse to world mouse so it stays under the cursor while panning
		Vector2 worldMouse = GetScreenToWorld2D(GetMousePosition(), camera);

		Vector2 snappedPos;
		snappedPos.x = floor(worldMouse.x / gridSize) * gridSize;
		snappedPos.y = floor(worldMouse.y / gridSize) * gridSize;

		if (GetMousePosition().x > 250) { // Still check screen pos for sidebar boundary
			Texture2D tex = editorAssets[selectedAssetIndex].preview;
			DrawTexture(tex, (int)snappedPos.x, (int)snappedPos.y, Fade(WHITE, 0.5f));
		}
	}

	EndMode2D();

	//Anything outside camera ex. UI and stuff

	UISystem::UpdateAndDraw(registry);
	if (isEditorMode) {
		EditorSystem::DrawAssetBrowser(editorAssets,currentBrowserPath,draggedAssetIndex);
		if (selectedEntity >= 0 && selectedEntity < MAX_ENTITIES) {
			if (registry.entityMasks[selectedEntity].none()) {
				selectedEntity = -1;
			}
			else {
				EditorSystem::DrawInspector(selectedEntity, registry, GetScreenWidth(), GetScreenHeight());
			}
		}
		else {
			EditorSystem::DrawInspector(selectedEntity, registry, GetScreenWidth(), GetScreenHeight());
		}
		if (draggedAssetIndex != -1) {
			Vector2 mPos = GetMousePosition();
			DrawTextureEx(editorAssets[draggedAssetIndex].preview,
				{ mPos.x - 20, mPos.y - 20 }, 0, 1.0f, Fade(WHITE, 0.6f));
		}
	}
	DebugSystem::Draw(registry,stats,GetScreenWidth());

	if (isEditorMode) {
		EditorSystem::DrawSettingsMenu(showSettings, settingsActiveTab, registry, this);
	}
	EndDrawing();
}

void Engine::ScanThemes() {
	TraceLog(LOG_INFO, "THEME_SYSTEM: Scanning assets/themes/ for .txt files...");

	themeFiles.clear();
	FilePathList files = LoadDirectoryFiles("assets/themes");

	if (files.count == 0) {
		TraceLog(LOG_WARNING, "THEME_SYSTEM: No files found in assets/themes/");
	}

	for (unsigned int i = 0; i < files.count; i++) {
		if (IsFileExtension(files.paths[i], ".txt")) {
			themeFiles.push_back(files.paths[i]);
			TraceLog(LOG_INFO, "THEME_SYSTEM: Found theme file: %s", files.paths[i]);
		}
	}

	TraceLog(LOG_INFO, "THEME_SYSTEM: Scan complete. Total themes found: %d", themeFiles.size());
	UnloadDirectoryFiles(files);
}

void Engine::ScanFonts() {
	fontFiles.clear();
	FilePathList files = LoadDirectoryFiles("assets/fonts");
	for (unsigned int i = 0; i < files.count; i++) {
		if (IsFileExtension(files.paths[i], ".ttf") || IsFileExtension(files.paths[i], ".otf")) {
			fontFiles.push_back(files.paths[i]);
			TraceLog(LOG_INFO, "FONT_SYSTEM: Found font: %s", files.paths[i]);
		}
	}
	UnloadDirectoryFiles(files);
}

void Engine::LoadEngineFont(const std::string& path) {
	if (!FileExists(path.c_str())) {
		TraceLog(LOG_WARNING, "FONT_SYSTEM: Font file not found: %s", path.c_str());
		return;
	}

	// Unload old font if it exists to prevent memory leaks
	if (engineFont.texture.id != 0) UnloadFont(engineFont);

	// Load new font (using size 32 for better scaling)
	engineFont = LoadFontEx(path.c_str(), 24, 0, 250);

	SetTextureFilter(engineFont.texture, TEXTURE_FILTER_BILINEAR);

	GuiSetFont(engineFont);

	ApplyTheme(lastThemePath);

	lastFontPath = path;

	TraceLog(LOG_INFO, "FONT_SYSTEM: Successfully loaded font: %s", path.c_str());
}

void Engine::ApplyTheme(const std::string& filePath) {
	TraceLog(LOG_INFO, "THEME_SYSTEM: Attempting to apply theme: %s", filePath.c_str());

	char* text = LoadFileText(filePath.c_str());
	if (text == NULL) {
		TraceLog(LOG_ERROR, "THEME_SYSTEM: Failed to load theme file text!");
		return;
	}

	std::string content(text);
	UnloadFileText(text);

	auto parseColor = [&](const std::string& key) -> Color {
		size_t pos = content.find(key + ": ");
		if (pos != std::string::npos) {
			int r, g, b, a;
			// Find the start of the numbers after the key and the space
			const char* start = content.c_str() + pos + key.length() + 2;
			if (sscanf(start, "%d, %d, %d, %d", &r, &g, &b, &a) == 4) {
				TraceLog(LOG_INFO, "THEME_SYSTEM: Parsed %s -> R:%d G:%d B:%d A:%d", key.c_str(), r, g, b, a);
				return { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
			}
		}
		TraceLog(LOG_WARNING, "THEME_SYSTEM: Could not parse key: %s", key.c_str());
		return MAGENTA; // Error color to make failures obvious
		};

	// Apply the parsed values
	currentTheme.background = parseColor("bg");
	currentTheme.panelBG = parseColor("panel");
	currentTheme.gridColor = parseColor("grid");
	currentTheme.accentColor = parseColor("accent");

	// Parse Font Size
	size_t fontPos = content.find("fontSize: ");
	if (fontPos != std::string::npos) {
		int fSize = 0;
		sscanf(content.c_str() + fontPos + 10, "%d", &fSize);
		currentTheme.fontSize = fSize;
		GuiSetStyle(DEFAULT, TEXT_SIZE, fSize);
		TraceLog(LOG_INFO, "THEME_SYSTEM: Set FontSize to %d", fSize);
	}

	// Apply to RayGui Global Styles
	GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(currentTheme.panelBG));
	GuiSetStyle(DEFAULT, LINE_COLOR, ColorToInt(currentTheme.accentColor));
	GuiSetStyle(LABEL, TEXT_COLOR_NORMAL, ColorToInt(currentTheme.accentColor));

	lastThemePath = filePath;
	TraceLog(LOG_INFO, "THEME_SYSTEM: Theme '%s' applied and saved to config.", filePath.c_str());
}

namespace EditorSystem {
	void DrawSettingsMenu(bool& open, int& activeTab, Registry& reg, Engine* engine) {
		if (!open) return;

		float sw = (float)GetScreenWidth();
		float sh = (float)GetScreenHeight();

		// 1. Draw Background
		DrawRectangleRec({ 0, 0, sw, sh }, Fade(BLACK, 0.85f));

		// 2. Window
		float winW = sw - 100;
		float winH = sh - 100;
		if (GuiWindowBox({ 50, 50, winW, winH }, "GLOBAL ENGINE SETTINGS")) {
			open = false;
		}

		// 3. Tabs
		const char* tabs[] = { "THEME", "GRAPHICS", "INPUT", "EDITOR" };
		GuiTabBar({ 60, 85, winW - 20, 30 }, tabs, 4, &activeTab);

		float contentX = 70;
		float contentY = 130;


		switch (activeTab) {
		case 0: // THEME TAB
			GuiLabel({ contentX, contentY, 200, 20 }, "Available Themes:");

			if (engine->themeFiles.empty()) {
				GuiLabel({ contentX, contentY + 30, 300, 20 }, "No themes found in vector.");
			}

			for (int i = 0; i < engine->themeFiles.size(); i++) {
				std::string path = engine->themeFiles[i];
				std::string fileName = GetFileName(path.c_str());

				// Draw Button
				if (GuiButton({ contentX, contentY + 30 + (i * 35), 250, 30 }, fileName.c_str())) {
					engine->ApplyTheme(path);
				}
			}
			break;

		case 1: // GRAPHICS
			GuiLabel({ contentX, contentY, 200, 20 }, "Graphics Settings (WIP)");
			break;
		case 2: // INPUT
			GuiLabel({ contentX, contentY, 200, 20 }, "Input Settings (WIP)");
			break;
		case 3: // EDITOR
		{
			GuiLabel({ contentX, contentY, 200, 20 }, "Editor Appearance:");

			float panelWidth = 350;
			float panelHeight = 300;

			// 1. SCROLL PANEL SETUP
			static Vector2 scrollPos = { 0, 0 };
			Rectangle viewScroll = { contentX, contentY + 60, panelWidth, panelHeight };
			Rectangle contentScroll = { 0, 0, panelWidth - 20, (float)engine->fontFiles.size() * 35 };

			GuiLabel({ contentX, contentY + 35, 200, 20 }, "[ Installed Fonts ]");

			Rectangle view = GuiScrollPanel(viewScroll, NULL, contentScroll, &scrollPos);

			// 2. FONT LIST WITH SCISSORING
			BeginScissorMode((int)view.x, (int)view.y, (int)view.width, (int)view.height);
			{
				for (int i = 0; i < (int)engine->fontFiles.size(); i++) {
					std::string path = engine->fontFiles[i];
					std::string fileName = GetFileName(path.c_str());

					Rectangle btnRect = {
						view.x + scrollPos.x + 5,
						view.y + scrollPos.y + (i * 35) + 5,
						panelWidth - 40,
						30
					};

					// Highlight the currently active font
					if (engine->lastFontPath == path) {
						GuiSetState(STATE_FOCUSED);
					}

					if (GuiButton(btnRect, fileName.c_str())) {
						engine->LoadEngineFont(path);
					}
					GuiSetState(STATE_NORMAL);
				}
			}
			EndScissorMode();

			// 3. TEXT SIZE SLIDER 
			static float currentFontSize = 18.0f;
			GuiLabel({ contentX + 380, contentY + 60, 200, 20 }, "UI Text Size:");
			if (GuiSlider({ contentX + 380, contentY + 85, 200, 20 }, "12", "32", currentFontSize, 12, 32)) {
				GuiSetStyle(DEFAULT, TEXT_SIZE, (int)currentFontSize);
			}

			// 4. ALIGNMENT
			static int alignment = 0; // 0-Left, 1-Center, 2-Right
			GuiLabel({ contentX + 380, contentY + 120, 200, 20 }, "Button Alignment:");
			if (GuiButton({ contentX + 380, contentY + 145, 60, 25 }, "LEFT")) GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
			if (GuiButton({ contentX + 445, contentY + 145, 60, 25 }, "CENTER")) GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);

			break;
		}
		}
	}
}