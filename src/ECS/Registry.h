#pragma once
#include <vector>
#include <bitset>
#include "Components.h"
#include "Entity.h" 

class Registry {
public:
    Registry() {
        transforms.resize(MAX_ENTITIES);
        sprites.resize(MAX_ENTITIES);
        velocities.resize(MAX_ENTITIES);
        inputComponents.resize(MAX_ENTITIES);
        uiComponents.resize(MAX_ENTITIES);
        scripts.resize(MAX_ENTITIES);
        entityMasks.resize(MAX_ENTITIES);
    }

    Entity CreateEntity() {
        if (nextEntity >= MAX_ENTITIES) {
            return MAX_ENTITIES;
        }

        return nextEntity++;
    }

    void AddComponent(Entity entity, TransformComponent c) {
        transforms[entity] = c;
        entityMasks[entity].set(COMP_TRANSFORM);
    }

    void AddComponent(Entity entity, SpriteComponent c) {
        sprites[entity] = c;
        entityMasks[entity].set(COMP_SPRITE);
    }

    void AddComponent(Entity entity, VelocityComponent c) {
        velocities[entity] = c;
        entityMasks[entity].set(COMP_VELOCITY);
    }

    void AddComponent(Entity entity, InputComponent c) {
        inputComponents[entity] = c;
        entityMasks[entity].set(COMP_INPUT);
    }

    void AddComponent(Entity entity, UIComponent c) {
        uiComponents[entity] = c;
        entityMasks[entity].set(COMP_UI);
    }

    void AddComponent(Entity entity, ScriptComponent c) {
        scripts[entity] = c;
        entityMasks[entity].set(COMP_SCRIPT);
    }

    bool HasComponent(Entity entity, ComponentType type) {
        return entityMasks[entity].test(type);
    }

    int GetAliveEntityCount() const {
        return (int)nextEntity;
    }
    
    std::vector<TransformComponent> transforms;
    std::vector<SpriteComponent> sprites;
    std::vector<VelocityComponent> velocities;
    std::vector<InputComponent> inputComponents;
    std::vector<UIComponent> uiComponents;
	std::vector<ScriptComponent> scripts;

    std::vector<std::bitset<COMP_COUNT>> entityMasks;

private:
    Entity nextEntity = 0;
};