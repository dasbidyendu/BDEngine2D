#include "RenderPipeline.h"
#include "ECS/Systems.h"

RenderPipeline::RenderPipeline(int w, int h) : width(w), height(h) {
    geometryTarget = LoadRenderTexture(width, height);
    finalTarget = LoadRenderTexture(width, height);
}

RenderPipeline::~RenderPipeline() {
    UnloadRenderTexture(geometryTarget);
    UnloadRenderTexture(finalTarget);
}

void RenderPipeline::Resize(int w, int h) {
    if (width == w && height == h) return;
    width = w;
    height = h;
    UnloadRenderTexture(geometryTarget);
    UnloadRenderTexture(finalTarget);
    geometryTarget = LoadRenderTexture(width, height);
    finalTarget = LoadRenderTexture(width, height);
}

void RenderPipeline::Execute(Registry& reg, ResourceManager& resources, Camera2D& camera, Color clearColor) {
    RenderSystem::UpdateShaders(reg, camera);
    ExecutePreProcess(reg, resources);
    ExecuteMainGeometry(reg, resources, camera, clearColor);
    ExecutePostProcess(reg, resources);
}

void RenderPipeline::ExecutePreProcess(Registry& reg, ResourceManager& resources) {
    // Left for future compute / radiance cascades
}

void RenderPipeline::ExecuteMainGeometry(Registry& reg, ResourceManager& resources, Camera2D& camera, Color clearColor) {
    BeginTextureMode(geometryTarget);
    ClearBackground(clearColor);
    BeginMode2D(camera);
    
    RenderSystem::Draw(reg);
    DebugSystem::PhysicsDebug(reg, camera);
    
    EndMode2D();
    EndTextureMode();
}

void RenderPipeline::ExecutePostProcess(Registry& reg, ResourceManager& resources) {
    BeginTextureMode(finalTarget);
    ClearBackground(BLACK);
    
    // Default passthrough
    Rectangle src = { 0, 0, (float)geometryTarget.texture.width, (float)-geometryTarget.texture.height };
    Rectangle dest = { 0, 0, (float)width, (float)height };
    
    DrawTexturePro(geometryTarget.texture, src, dest, {0, 0}, 0.0f, WHITE);
    
    EndTextureMode();
}
