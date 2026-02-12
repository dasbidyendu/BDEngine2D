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
            return MAX_ENTITIES;
        }
        Entity e = nextEntity++;
		activeEntities.push_back(e);
        return e;
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

    void AddComponent(Entity entity, UICanvasComponent c) {
        uiCanvases[entity] = c;
        entityMasks[entity].set(COMP_UICANVAS);
	}
    
    void AddComponent(Entity entity, SpriteAnimationComponent c) {
        spriteAnimations[entity] = c;
        entityMasks[entity].set(COMP_SPRITE_ANIMATION);
	}

    void AddComponent(Entity entity, RigidPhysicsComponent c) {
        rigidPhysicsComponents[entity] = c;
        entityMasks[entity].set(COMP_RIGIDPHYSICS);
	}

    void AddComponent(Entity entity, CircleColliderComponent c) {
        circleColliders[entity] = c;
        entityMasks[entity].set(COMP_CIRCLECOLLIDER);
    }

    void AddComponent(Entity entity, BoxColliderComponent c) {
        boxColliders[entity] = c;
        entityMasks[entity].set(COMP_BOXCOLLIDER);
	}

    bool HasComponent(Entity entity, ComponentType type) {
        return entityMasks[entity].test(type);
    }

    int GetAliveEntityCount() const {
        return (int)nextEntity;
    }
    
    void Clear() {
        for (int i = 0; i < MAX_ENTITIES; i++) {
            entityMasks[i].reset();
        }
		activeEntities.clear();
        nextEntity = 0;
    }

    void SetNextEntity(Entity val) {
        nextEntity = val;
    }

    std::vector<Entity> activeEntities;
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