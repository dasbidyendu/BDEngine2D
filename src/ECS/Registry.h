#pragma once
#include "Components.h"
#include "Entity.h"

#include <bitset>
#include <vector>

class Registry {
public:
  Registry() {
    names.resize(MAX_ENTITIES);
    transforms.resize(MAX_ENTITIES);
    sprites.resize(MAX_ENTITIES);
    velocities.resize(MAX_ENTITIES);
    inputComponents.resize(MAX_ENTITIES);
    uiComponents.resize(MAX_ENTITIES);
    uiCanvases.resize(MAX_ENTITIES);
    scripts.resize(MAX_ENTITIES);
    entityMasks.resize(MAX_ENTITIES);
    spriteAnimations.resize(MAX_ENTITIES);
    rigidPhysicsComponents.resize(MAX_ENTITIES);
    circleColliders.resize(MAX_ENTITIES);
    boxColliders.resize(MAX_ENTITIES);
  }

  Entity CreateEntity() {
    if (nextEntity >= MAX_ENTITIES) {
      return -1;
    }
    Entity e = nextEntity++;
    activeEntities.push_back(e);

    NameComponent defaultName;
    defaultName.name = "Entity " + std::to_string(e);
    AddComponent(e, defaultName);

    return e;
  }

  void DestroyEntity(Entity entity) {
    if (entity < 0 || entity >= MAX_ENTITIES)
      return;

    entityMasks[entity].reset();

    auto it = std::find(activeEntities.begin(), activeEntities.end(), entity);

    if (it != activeEntities.end()) {
      *it = activeEntities.back();
      activeEntities.pop_back();
    }
  }

  void AddComponent(Entity entity, const NameComponent &c) {
    names[entity] = c;
    entityMasks[entity].set(COMP_NAME);
  }

  void AddComponent(Entity entity, const TransformComponent &c) {
    transforms[entity] = c;
    entityMasks[entity].set(COMP_TRANSFORM);
  }

  void AddComponent(Entity entity, const SpriteComponent &c) {
    sprites[entity] = c;
    entityMasks[entity].set(COMP_SPRITE);
  }

  void AddComponent(Entity entity, const VelocityComponent &c) {
    velocities[entity] = c;
    entityMasks[entity].set(COMP_VELOCITY);
  }

  void AddComponent(Entity entity, const InputComponent &c) {
    inputComponents[entity] = c;
    entityMasks[entity].set(COMP_INPUT);
  }

  void AddComponent(Entity entity, const UIComponent &c) {
    uiComponents[entity] = c;
    entityMasks[entity].set(COMP_UI);
  }

  void AddComponent(Entity entity, const ScriptComponent &c) {
    scripts[entity] = c;
    entityMasks[entity].set(COMP_SCRIPT);
  }

  void AddComponent(Entity entity, const UICanvasComponent &c) {
    uiCanvases[entity] = c;
    entityMasks[entity].set(COMP_UICANVAS);
  }

  void AddComponent(Entity entity, const SpriteAnimationComponent &c) {
    spriteAnimations[entity] = c;
    entityMasks[entity].set(COMP_SPRITE_ANIMATION);
  }

  void AddComponent(Entity entity, const RigidPhysicsComponent &c) {
    rigidPhysicsComponents[entity] = c;
    entityMasks[entity].set(COMP_RIGIDPHYSICS);
  }

  void AddComponent(Entity entity, const CircleColliderComponent &c) {
    circleColliders[entity] = c;
    entityMasks[entity].set(COMP_CIRCLECOLLIDER);
  }

  void AddComponent(Entity entity, const BoxColliderComponent &c) {
    boxColliders[entity] = c;
    entityMasks[entity].set(COMP_BOXCOLLIDER);
  }

  bool HasComponent(Entity entity, ComponentType type) {
    return entityMasks[entity].test(type);
  }

  bool HasComponents(Entity e, const std::bitset<COMP_COUNT> &mask) {
    return (entityMasks[e] & mask) == mask;
  }

  int GetAliveEntityCount() const { return (int)nextEntity; }

  void Clear() {
    for (int i = 0; i < MAX_ENTITIES; i++) {
      entityMasks[i].reset();
    }
    activeEntities.clear();
    nextEntity = 0;
  }

  void SetNextEntity(Entity val) { nextEntity = val; }

  std::vector<Entity> activeEntities;
  std::vector<NameComponent> names;
  std::vector<TransformComponent> transforms;
  std::vector<SpriteComponent> sprites;
  std::vector<VelocityComponent> velocities;
  std::vector<InputComponent> inputComponents;
  std::vector<UICanvasComponent> uiCanvases;
  std::vector<UIComponent> uiComponents;
  std::vector<ScriptComponent> scripts;
  std::vector<SpriteAnimationComponent> spriteAnimations;
  std::vector<RigidPhysicsComponent> rigidPhysicsComponents;
  std::vector<CircleColliderComponent> circleColliders;
  std::vector<BoxColliderComponent> boxColliders;
  std::vector<std::bitset<COMP_COUNT>> entityMasks;

private:
  Entity nextEntity = 0;
};