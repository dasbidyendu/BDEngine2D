#pragma once
#include "raylib.h"
#include "ECS/Registry.h"
#include "Managers/ResourceManager.h"

enum class RenderPassType {
    PRE_PROCESS,
    MAIN_GEOMETRY,
    POST_PROCESS
};

class RenderPipeline {
public:
    RenderPipeline(int width, int height);
    ~RenderPipeline();

    void Resize(int width, int height);

    void Execute(Registry& reg, ResourceManager& resources, Camera2D& camera, Color clearColor);

    RenderTexture2D GetOutputTexture() const { return finalTarget; }

private:
    void ExecutePreProcess(Registry& reg, ResourceManager& resources);
    void ExecuteMainGeometry(Registry& reg, ResourceManager& resources, Camera2D& camera, Color clearColor);
    void ExecutePostProcess(Registry& reg, ResourceManager& resources);

    int width, height;
    RenderTexture2D geometryTarget;
    RenderTexture2D finalTarget;
};
