#pragma once
#include "raylib.h"
#include <vector>
#include <string>

enum class NeighborCondition : int {
    IGNORE = 0,
    OCCUPIED = 1,
    EMPTY = 2
};

struct TileRule {
    int outputIndex;
    NeighborCondition neighbors[8]; // N, NE, E, SE, S, SW, W, NW
};

struct TileConfig {
    int index;
    std::string name;
    std::vector<TileRule> rules;
    bool isRuleTile = false;
};

class TileSet {
public:
    std::string name;
    std::string texturePath;
    Texture2D texture;
    int tileSize;
    
    std::vector<Rectangle> sourceRects;
    std::vector<TileConfig> tileConfigs;

    void Load(const std::string& path);
    void Save(const std::string& path);
};
