#pragma once

#include "raylib.h"
#include "ECS/Registry.h"
#include "raymath.h"
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

class ThreadPool {
public:
	ThreadPool(size_t numThreads) :stop(false) {
		for (size_t i = 0; i < numThreads; i++) {
			workers.emplace_back([this]() {
				while (true) {
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock(this->queueMutex);
						this->condition.wait(lock, [this]() { return this->stop || !this->tasks.empty(); });
						if (this->stop && this->tasks.empty())
							return;
						task = std::move(this->tasks.front());
						this->tasks.pop();
					}
					task();
				}
			});
		}
	}

	int GetWorkerCount() const {
		return workers.size();
	}

	template<class F>
	void Enqueue(F&& f) {
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			tasks.emplace(std::forward<F>(f));
		}
		condition.notify_one();
	}

	~ThreadPool() {
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			stop = true;
		}
		condition.notify_all();
		for (std::thread& worker : workers)
			worker.join();
	}
private:
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;
	std::mutex queueMutex;
	std::condition_variable condition;
	bool stop;
};

struct GridCell {
	std::vector<Entity> entities;
};

class SpatialHashGrid {
public:
	int cellSize;
	int col, rows;
	std::vector<GridCell> cells;

	SpatialHashGrid(int worldWidth, int worldHeight, int cellSize)
		: cellSize(cellSize) {
		col = (worldWidth + cellSize - 1) / cellSize;
		rows = (worldHeight + cellSize - 1) / cellSize;
		cells.resize(col * rows);
	}

	void Clear() {
		for (auto& cell : cells) {
			cell.entities.clear();
		}
	}
	//circle
	void AddEntity(Entity e, Vector2 position, float radius) {
		int minX = (int)((position.x - radius) / cellSize);
		int maxX = (int)((position.x + radius) / cellSize);
		int minY = (int)((position.y - radius) / cellSize);
		int maxY = (int)((position.y + radius) / cellSize);

		for (int y = minY; y <= maxY; y++) {
			for (int x = minX; x <= maxX; x++) {
				if (x >= 0 && x < col && y >= 0 && y < rows) {
					cells[y * col + x].entities.push_back(e);
				}
			}
		}
	}
	//box
	void AddEntity(Entity e, Vector2 position, float width, float height) {
		float halfW = width * 0.5f;
		float halfH = height * 0.5f;
		int minX = (int)((position.x - halfW) / cellSize);
		int maxX = (int)((position.x + halfW) / cellSize);
		int minY = (int)((position.y - halfH) / cellSize);
		int maxY = (int)((position.y + halfH) / cellSize);

		for (int y = minY; y <= maxY; y++) {
			for (int x = minX; x <= maxX; x++) {
				if (x >= 0 && x < col && y >= 0 && y < rows) {
					cells[y * col + x].entities.push_back(e);
				}
			}
		}
	}
};

class PhysicsSystem {
public:
    struct Command {
        enum Type { DESTROY, ADD_VELOCITY, SET_POS };
        Type type;
        Entity entity;
        Vector2 value;
    };

    std::vector<Command> commandBuffer;
    std::mutex commandMutex;
    ThreadPool pool{ std::thread::hardware_concurrency() };

    void ProcessCommands(Registry& reg) {
        std::lock_guard<std::mutex> lock(commandMutex);
        for (auto& cmd : commandBuffer) {
            if (cmd.type == Command::DESTROY) {
                auto it = std::find(reg.activeEntities.begin(), reg.activeEntities.end(), cmd.entity);
                if (it != reg.activeEntities.end()) {
                    reg.entityMasks[cmd.entity].reset();
                    *it = reg.activeEntities.back();
                    reg.activeEntities.pop_back();
                }
            }
            else if (cmd.type == Command::ADD_VELOCITY) {
                if (reg.HasComponent(cmd.entity, COMP_VELOCITY)) {
                    reg.velocities[cmd.entity].speed = Vector2Add(reg.velocities[cmd.entity].speed, cmd.value);
                }
            }
            else if (cmd.type == Command::SET_POS) {
                if (reg.HasComponent(cmd.entity, COMP_TRANSFORM)) {
                    reg.transforms[cmd.entity].position = cmd.value;
                }
            }
        }
        commandBuffer.clear();
    }

    void ResolveManifold(Entity a, Entity b, Vector2 normal, float penetration, Registry& reg) {
        auto& transA = reg.transforms[a];
        auto& transB = reg.transforms[b];

        bool hasRBA = reg.HasComponent(a, COMP_RIGIDPHYSICS);
        bool hasRBB = reg.HasComponent(b, COMP_RIGIDPHYSICS);

        bool staticA = hasRBA && reg.rigidPhysicsComponents[a].isStatic;
        bool staticB = hasRBB && reg.rigidPhysicsComponents[b].isStatic;

        Vector2 correction = Vector2Scale(normal, penetration / 2.0f);
        if (!staticA) transA.position = Vector2Subtract(transA.position, correction);
        if (!staticB) transB.position = Vector2Add(transB.position, correction);

        if (hasRBA && hasRBB) {
            auto& rbA = reg.rigidPhysicsComponents[a];
            auto& rbB = reg.rigidPhysicsComponents[b];
            auto& velA = reg.velocities[a].speed;
            auto& velB = reg.velocities[b].speed;

            Vector2 rv = Vector2Subtract(velB, velA);
            float velAlongNormal = Vector2DotProduct(rv, normal);

            if (velAlongNormal < 0) {
                float e = fminf(rbA.restitution, rbB.restitution);
                float j = -(1 + e) * velAlongNormal;

                float invMassA = (rbA.mass > 0 && !staticA) ? 1.0f / rbA.mass : 0;
                float invMassB = (rbB.mass > 0 && !staticB) ? 1.0f / rbB.mass : 0;

                if (invMassA + invMassB > 0) {
                    j /= (invMassA + invMassB);
                    Vector2 impulse = Vector2Scale(normal, j);

                    if (!staticA) velA = Vector2Subtract(velA, Vector2Scale(impulse, invMassA));
                    if (!staticB) velB = Vector2Add(velB, Vector2Scale(impulse, invMassB));
                }
            }
        }
    }

    bool CheckCircleCollision(Entity a, Entity b, Registry& reg) {
        Vector2 posA = Vector2Add(reg.transforms[a].position, reg.circleColliders[a].offset);
        Vector2 posB = Vector2Add(reg.transforms[b].position, reg.circleColliders[b].offset);
        float dist = Vector2Distance(posA, posB);
        float radiusSum = reg.circleColliders[a].radius + reg.circleColliders[b].radius;

        if (radiusSum > dist) {
            Vector2 normal = (dist > 0) ? Vector2Scale(Vector2Subtract(posB, posA), 1.0f / dist) : Vector2{ 0, -1 };
            ResolveManifold(a, b, normal, radiusSum - dist, reg);
            return true;
        }
        return false;
    }
    bool CheckBoxCollision(Entity a, Entity b, Registry& reg) {
        Vector2 posA = Vector2Add(reg.transforms[a].position, reg.boxColliders[a].offset);
        Vector2 posB = Vector2Add(reg.transforms[b].position, reg.boxColliders[b].offset);

        auto& boxA = reg.boxColliders[a];
        auto& boxB = reg.boxColliders[b];

        float hwa = boxA.size.x * 0.5f;
        float hha = boxA.size.y * 0.5f;
        float hwb = boxB.size.x * 0.5f;
        float hhb = boxB.size.y * 0.5f;

        float dx = posB.x - posA.x;
        float dy = posB.y - posA.y;

        float overlapX = hwa + hwb - fabsf(dx);
        float overlapY = hha + hhb - fabsf(dy);

        if (overlapX > 0 && overlapY > 0) {
            Vector2 normal = { 0, 0 };
            float penetration = 0;

            if (overlapX < overlapY) {
                normal = { (dx > 0) ? 1.0f : -1.0f, 0 };
                penetration = overlapX;
            }
            else {
                normal = { 0, (dy > 0) ? 1.0f : -1.0f };
                penetration = overlapY;
            }

            ResolveManifold(a, b, normal, penetration, reg);
            return true;
        }
        return false;
    }

    bool CheckCircleBoxCollision(Entity circle, Entity box, Registry& reg) {
        Vector2 cPos = Vector2Add(reg.transforms[circle].position, reg.circleColliders[circle].offset);
        Vector2 bPos = Vector2Add(reg.transforms[box].position, reg.boxColliders[box].offset);

        auto& circ = reg.circleColliders[circle];
        auto& bo = reg.boxColliders[box];

        float closestX = Clamp(cPos.x, bPos.x - bo.size.x * 0.5f, bPos.x + bo.size.x * 0.5f);
        float closestY = Clamp(cPos.y, bPos.y - bo.size.y * 0.5f, bPos.y + bo.size.y * 0.5f);

        float distance = Vector2Distance(cPos, { closestX, closestY });

        if (distance < circ.radius) {
            Vector2 normal;

            if (distance > 0) {
                normal = Vector2Normalize(Vector2Subtract({ closestX, closestY }, cPos));
            }
            else {
                float dx = cPos.x - bPos.x;
                float dy = cPos.y - bPos.y;
                float overlapX = (bo.size.x * 0.5f) - fabsf(dx);
                float overlapY = (bo.size.y * 0.5f) - fabsf(dy);

                if (overlapX < overlapY)
                    normal = { (dx > 0) ? 1.0f : -1.0f, 0 };
                else
                    normal = { 0, (dy > 0) ? 1.0f : -1.0f };
            }

            float penetration = circ.radius - distance;

            ResolveManifold(circle, box, normal, penetration, reg);
            return true;
        }
        return false;
    }
    void UpdatePhysics(float dt, Registry& reg, SpatialHashGrid& grid) {
        std::bitset<COMP_COUNT> physMask;
        physMask.set(COMP_TRANSFORM);
        physMask.set(COMP_VELOCITY);

        grid.Clear();

        for (Entity e : reg.activeEntities) {
            if (reg.HasComponent(e, COMP_CIRCLECOLLIDER)) reg.circleColliders[e].isColliding = false;
            if (reg.HasComponent(e, COMP_BOXCOLLIDER)) reg.boxColliders[e].isColliding = false;

            if (reg.HasComponents(e, physMask)) {
                auto& pos = reg.transforms[e].position;
                auto& vel = reg.velocities[e].speed;

                pos = Vector2Add(pos, Vector2Scale(vel, dt));

                if (reg.HasComponent(e, COMP_RIGIDPHYSICS)) {
                    auto& rb = reg.rigidPhysicsComponents[e];
                    if (rb.affectedByGravity && !rb.isStatic) {
                        vel.y += 9.81f * rb.gravityScale * dt;
                    }

                    if (reg.HasComponent(e, COMP_CIRCLECOLLIDER))
                        grid.AddEntity(e, pos, reg.circleColliders[e].radius);
                    else if (reg.HasComponent(e, COMP_BOXCOLLIDER))
                        grid.AddEntity(e, pos, reg.boxColliders[e].size.x * 0.5f);
                }
            }
        }

        int totalCells = (int)grid.cells.size();
        int workerCount = pool.GetWorkerCount();
        int batchSize = (totalCells + workerCount - 1) / workerCount;
        std::atomic<int> tasksRemaining(0);

        for (int i = 0; i < totalCells; i += batchSize) {
            int end = std::min(i + batchSize, totalCells);
            tasksRemaining++;

            pool.Enqueue([this, i, end, &reg, &grid, &tasksRemaining]() {
                for (int c = i; c < end; c++) {
                    auto& cell = grid.cells[c];
                    if (cell.entities.size() < 2) continue;

                    for (size_t a_idx = 0; a_idx < cell.entities.size(); a_idx++) {
                        for (size_t b_idx = a_idx + 1; b_idx < cell.entities.size(); b_idx++) {
                            Entity a = cell.entities[a_idx];
                            Entity b = cell.entities[b_idx];

                            std::lock_guard<std::mutex> lock(commandMutex);

                            bool hasCircA = reg.HasComponent(a, COMP_CIRCLECOLLIDER);
                            bool hasCircB = reg.HasComponent(b, COMP_CIRCLECOLLIDER);
                            bool hasBoxA = reg.HasComponent(a, COMP_BOXCOLLIDER);
                            bool hasBoxB = reg.HasComponent(b, COMP_BOXCOLLIDER);

                            if (hasCircA && hasCircB) {
                                if (CheckCircleCollision(a, b, reg)) {
                                    reg.circleColliders[a].isColliding = true;
                                    reg.circleColliders[b].isColliding = true;
                                }
                            }
                            else if (hasBoxA && hasBoxB) {
                                if (CheckBoxCollision(a, b, reg)) {
                                    reg.boxColliders[a].isColliding = true;
                                    reg.boxColliders[b].isColliding = true;
                                }
                            }
                            else if (hasCircA && hasBoxB) {
                                if (CheckCircleBoxCollision(a, b, reg)) {
                                    reg.circleColliders[a].isColliding = true;
                                    reg.boxColliders[b].isColliding = true;
                                }
                            }
                            else if (hasBoxA && hasCircB) {
                                if (CheckCircleBoxCollision(b, a, reg)) {
                                    reg.boxColliders[a].isColliding = true;
                                    reg.circleColliders[b].isColliding = true;
                                }
                            }
                        }
                    }
                }
                tasksRemaining--;
                });
        }

        while (tasksRemaining > 0) std::this_thread::yield();

        ProcessCommands(reg);
    }
};
