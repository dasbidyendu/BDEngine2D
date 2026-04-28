#pragma once
#include "ECS/Entity.h"
#include "raylib.h"
#include <string>
#include <vector>

class Engine;
class Registry;
struct AssetEntry;

struct ScriptEditorTab {
  std::string filePath;
  std::string content;
  bool dirty = false;
  bool open = true;
};

class Editor {
public:
  Editor(Engine *engine) : owner(engine) {};

  void Update();
  void Render();
  void DrawTransportBar();
  void DrawTopBar();
  void DrawUIPalette();
  void DrawUIViewport();
  void DrawSceneView();
  void DrawGameView();
  void DrawMenuBar();
  void LoadScene(const std::string &path);

  // Script editor
  std::vector<ScriptEditorTab> openScriptTabs;
  void OpenScriptEditor(const std::string &path);
  void DrawScriptEditors();
  void DrawConsole();

  bool showConsole = true;
  char consoleSearch[128] = "";
  bool consoleAutoScroll = true;
  bool filterInfo = true;
  bool filterWarn = true;
  bool filterError = true;
  bool filterSuccess = true;
  bool filterRaylib = false;

  std::string currentBrowserPath = "assets";
  std::string lastPath = "";
  int browserActiveTab = 0;
  int draggedAssetIndex = -1;

  enum EditorMode { MODE_WORLD, MODE_UI_EDITOR } currentMode = MODE_WORLD;
  enum GizmoMode { GIZMO_TRANSLATE, GIZMO_ROTATE, GIZMO_SCALE } currentGizmoMode = GIZMO_TRANSLATE;
  enum GizmoAxis { AXIS_NONE, AXIS_X, AXIS_Y, AXIS_CENTER } activeGizmoAxis = AXIS_NONE, hoveredGizmoAxis = AXIS_NONE;

  Entity activeCanvasId = -1;

  Vector2 sceneViewPos = {0, 0};
  Vector2 sceneViewSize = {0, 0};
  bool isDraggingGizmo = false;
  Vector2 gizmoDragStartPos = {0, 0};
  Vector2 gizmoDragStartValue = {0, 0};
  float gizmoDragStartRotation = 0.0f;
  
  // Tiling Editor State
  enum TilingMode { TILE_PAINT, TILE_ERASE, TILE_SELECT } currentTilingMode = TILE_PAINT;
  int selectedTileIndex = 0;
  bool showTilingManager = true;
  Entity activeTilemapEntity = -1;
  int brushSize = 1;

  //Editor State
  bool showExitModal = false;

  void DrawTilingManager();

private:
  Engine *owner;
  /*void CreateMainDockSpace();*/
};

namespace EditorSystem {
void DrawInspector(Entity e, Registry &reg, int screenWidth, int screenHeight,
                   Engine *engine);
void DrawSettingsMenu(bool &open, int &activeTab, Registry &reg,
                      Engine *engine);
void DrawGrid(int gridSize, Camera2D camera, int screenWidth, int screenHeight,
              Color color);
void DrawAssetBrowser(std::vector<AssetEntry> &assets, std::string &currentPath,
                      int &draggedIndex, Editor *editor);
void DrawUIInspector(Entity e, Registry &reg, float xPos, float &currentY,
                     float panelWidth, Engine *engine);
} // namespace EditorSystem