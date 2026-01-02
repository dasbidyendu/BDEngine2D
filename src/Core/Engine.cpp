#define _CRT_SECURE_NO_WARNINGS
#include "Engine.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

Engine::Engine(int width, int height, const std::string& title)
	:screenWidth(width),screenHeight(height),windowTitle(title) {
	
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);

	InitWindow(screenWidth, screenHeight, windowTitle.c_str());
	SetTargetFPS(0);
	isRunning = true;

	camera.target = { 0.0f, 0.0f };
	camera.offset = { 0.0f, 0.0f };
	camera.rotation = 0.0f;
	camera.zoom = 1.0f;

	GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
	/*assets.LoadTextureAsset("player", "assets/test.png");*/
}

Engine::~Engine() {
	CloseWindow();
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

	// Update Stats Data
	stats.frameCount++;

	// Toggle Settings with F1 (Check this first!)
	if (IsKeyPressed(KEY_F1)) showSettings = !showSettings;

	// Toggle Editor Mode with TAB
	if (IsKeyPressed(KEY_TAB)) isEditorMode = !isEditorMode;

	// Toggle Grid with G
	if (isEditorMode && IsKeyPressed(KEY_G)) {
		showGrid = !showGrid;
	}

	bool mouseInSidebar = (isEditorMode && mousePos.x < 250);
	bool mouseInInspector = (isEditorMode && mousePos.x > (screenWidth - 250));
	bool mouseInUI = mouseInSidebar || mouseInInspector;

	
	if (!showSettings) {

		//  Game Input & Camera Controls
		if (!mouseInSidebar) {
			InputSystem::Update(registry);
			ControlSystem::Update(registry);

			if (isEditorMode) {
				// Panning (Right Mouse Button)
				if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
					Vector2 delta = GetMouseDelta();
					camera.target.x -= delta.x / camera.zoom;
					camera.target.y -= delta.y / camera.zoom;
				}

				// Zooming (Mouse Wheel)
				float wheel = GetMouseWheelMove();
				if (wheel != 0) {
					Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
					camera.offset = GetMousePosition();
					camera.target = mouseWorldPos;
					camera.zoom += wheel * zoomSensitivity;

					// Clamp Zoom
					if (camera.zoom < 0.1f) camera.zoom = 0.1f;
					if (camera.zoom > 5.0f) camera.zoom = 5.0f;
				}
			}
		}

		MovementSystem::Update(registry, dt);

		//  Editor Logic 
		if (isEditorMode) {
			// Select Asset from Sidebar
			if (mouseInSidebar && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
				int idx = (int)((mousePos.y - 50) / 55);
				if (idx >= 0 && idx < editorAssets.size()) {
					selectedAssetIndex = idx;
				}
			}
			// Place Asset in World
			else if (selectedAssetIndex != -1 && !mouseInSidebar && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
				Vector2 worldMouse = GetScreenToWorld2D(GetMousePosition(), camera);
				Vector2 snappedPos;
				snappedPos.x = floor(worldMouse.x / gridSize) * gridSize;
				snappedPos.y = floor(worldMouse.y / gridSize) * gridSize;

				Entity newEntity = registry.CreateEntity();
				registry.AddComponent(newEntity, TransformComponent{ snappedPos, {1,1}, 0.0f });
				registry.AddComponent(newEntity, SpriteComponent{ assets.GetTexture(editorAssets[selectedAssetIndex].name), WHITE });

				selectedAssetIndex = -1; // Deselect after placing
			}
			// Select Entity in World
			else if (!mouseInUI && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
				Vector2 worldMouse = GetScreenToWorld2D(GetMousePosition(), camera);
				selectedEntity = -1; // Reset selection first

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

			// Right Click to Cancel Placement
			if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) selectedAssetIndex = -1;

			// Delete Entity
			if (selectedEntity != -1) {
				if (IsKeyPressed(KEY_DELETE)) {
					registry.entityMasks[selectedEntity].reset();
					selectedEntity = -1;
				}
			}
		}
	}

	// Toggle VSync with F3
	if (IsKeyPressed(KEY_F3)) {
		if (IsWindowState(FLAG_VSYNC_HINT)) {
			ClearWindowState(FLAG_VSYNC_HINT);
			SetTargetFPS(0);
		}
		else {
			SetWindowState(FLAG_VSYNC_HINT);
		}
	}

	// Toggle Stats with F2
	if (IsKeyPressed(KEY_F2)) stats.Toggle();
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
		EditorSystem::DrawAssetBrowser(editorAssets);
		if (selectedEntity >= 0 && selectedEntity < MAX_ENTITIES) {
			if (registry.entityMasks[selectedEntity].none()) {
				selectedEntity = -1;
			}
			else {
				EditorSystem::DrawInspector(selectedEntity, registry, screenWidth, screenHeight);
			}
		}
		else {
			EditorSystem::DrawInspector(selectedEntity, registry, screenWidth, screenHeight);
		}
	}
	DebugSystem::Draw(registry,stats,screenWidth);

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

	TraceLog(LOG_INFO, "THEME_SYSTEM: Theme '%s' applied successfully.", filePath.c_str());
}

namespace EditorSystem {
	void DrawSettingsMenu(bool& open, int& activeTab, Registry& reg, Engine* engine) {
		if (!open) return;

		float sw = (float)GetScreenWidth();
		float sh = (float)GetScreenHeight();

		// 1. Draw Background
		DrawRectangleRec({ 0, 0, sw, sh }, Fade(BLACK, 0.85f));

		// 2. Window
		if (GuiWindowBox({ 50, 50, sw - 100, sh - 100 }, "GLOBAL ENGINE SETTINGS")) {
			open = false;
		}

		// 3. Tabs
		const char* tabs[] = { "THEME", "GRAPHICS", "INPUT", "EDITOR" };
		GuiTabBar({ 60, 85, sw - 120, 30 }, tabs, 4, &activeTab);

		float contentX = 70;
		float contentY = 130;

		// --- (REMOVE LATER) ---
		char debugMsg[128];
		sprintf(debugMsg, "Active Tab: %d | Theme Count: %d", activeTab, (int)engine->themeFiles.size());
		DrawText(debugMsg, (int)contentX, (int)sh - 50, 20, RED);
		// ------------------------------------

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
		}
	}
}