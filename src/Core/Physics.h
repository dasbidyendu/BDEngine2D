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
};

struct PhysicsState {
	Vector2 position;
	Vector2 velocity;
};

class PhysicsSystem {
public:
	PhysicsState nextStates[MAX_ENTITIES];

	struct Command {
		enum Type {DESTROY,ADD_VELOCITY,SET_POS};
		Type type;
		Entity entity;
		Vector2 value; 
	};
	std::vector<Command> commandBuffer;
	std::mutex commandMutex;
	ThreadPool pool{ std::thread::hardware_concurrency() };

	void StartFrame(Registry& reg) {
		for (Entity e = 0; e < MAX_ENTITIES; e++) {
			if (reg.HasComponent(e, COMP_TRANSFORM)) {
				nextStates[e].position = reg.transforms[e].position;
				if (reg.HasComponent(e, COMP_VELOCITY))
					nextStates[e].velocity = reg.velocities[e].speed;
				else
					nextStates[e].velocity = { 0,0 };
			}
		}
	}

	void SyncPoint(Registry& reg) {

		for (Entity e = 0; e < MAX_ENTITIES; e++) {
			if (reg.HasComponent(e, COMP_TRANSFORM)) {
				reg.transforms[e].position = nextStates[e].position;
				if (reg.HasComponent(e, COMP_VELOCITY))
					reg.velocities[e].speed = nextStates[e].velocity;
			}
		}

		for (auto& cmd:commandBuffer) {
			if (cmd.type == Command::DESTROY) {
				reg.entityMasks[cmd.entity].reset();
			}
			else if(cmd.type==Command::ADD_VELOCITY) {
				if (reg.HasComponent(cmd.entity, COMP_VELOCITY)) {
					nextStates[cmd.entity].velocity = Vector2Add(nextStates[cmd.entity].velocity, cmd.value);
				}
			}
			else if (cmd.type == Command::SET_POS) {
				if (reg.HasComponent(cmd.entity, COMP_TRANSFORM)) {
					nextStates[cmd.entity].position = cmd.value;
				}
			}
		}
		commandBuffer.clear();
	}
	bool CheckCircleCollision(Entity a, Entity b, Registry& reg) {
		auto& posA = nextStates[a].position;
		auto& posB = nextStates[b].position;
		auto& colA = reg.circleColliders[a];
		auto& colB = reg.circleColliders[b];

		float dist = Vector2Distance(posA, posB);
		float radiusSum = colA.radius + colB.radius;
		float overlap = radiusSum - dist;

		if (overlap > 0) {
			Vector2 collisionNormal;
			if (dist > 0) {
				collisionNormal = Vector2Scale(Vector2Subtract(posB, posA), 1.0f / dist);
			}
			else {
				collisionNormal = { 0, -1 };
			}

			Vector2 correction = Vector2Scale(collisionNormal, overlap / 2.0f);

			bool staticA = reg.HasComponent(a, COMP_RIGIDPHYSICS) && reg.rigidPhysicsComponents[a].isStatic;
			bool staticB = reg.HasComponent(b, COMP_RIGIDPHYSICS) && reg.rigidPhysicsComponents[b].isStatic;

			if (!staticA) nextStates[a].position = Vector2Subtract(nextStates[a].position, correction);
			if (!staticB) nextStates[b].position = Vector2Add(nextStates[b].position, correction);

			if (reg.HasComponent(a, COMP_RIGIDPHYSICS) && reg.HasComponent(b, COMP_RIGIDPHYSICS)) {
				auto& rbA = reg.rigidPhysicsComponents[a];
				auto& rbB = reg.rigidPhysicsComponents[b];

				Vector2 rv = Vector2Subtract(nextStates[b].velocity, nextStates[a].velocity);
				float velAlongNormal = Vector2DotProduct(rv, collisionNormal);

				if (velAlongNormal < 0) {
					float e = fminf(rbA.restitution, rbB.restitution);
					float j = -(1 + e) * velAlongNormal;

					float invMassA = (rbA.mass > 0) ? 1.0f / rbA.mass : 0;
					float invMassB = (rbB.mass > 0) ? 1.0f / rbB.mass : 0;

					if (invMassA + invMassB > 0) {
						j /= (invMassA + invMassB);
						Vector2 impulse = Vector2Scale(collisionNormal, j);

						if (!staticA) {
							nextStates[a].velocity = Vector2Subtract(nextStates[a].velocity, Vector2Scale(impulse, invMassA));
						}
						if (!staticB) {
							nextStates[b].velocity = Vector2Add(nextStates[b].velocity, Vector2Scale(impulse, invMassB));
						}
					}
				}
			}
			return true;
		}
		return false;
	}
	void UpdatePhysics(float dt,Registry& reg,SpatialHashGrid& grid) {
		for (Entity e = 0; e < MAX_ENTITIES; e++) {
			if (reg.HasComponent(e, COMP_CIRCLECOLLIDER))
				reg.circleColliders[e].isColliding = false;
		}
		grid.Clear();
		for (Entity e = 0; e < MAX_ENTITIES; e++) {
			if (reg.HasComponent(e, COMP_TRANSFORM) && reg.HasComponent(e, COMP_VELOCITY)) {
				nextStates[e].position = Vector2Add(nextStates[e].position, Vector2Scale(nextStates[e].velocity, dt));
				if (reg.HasComponent(e, COMP_RIGIDPHYSICS)) {
					grid.AddEntity(e, nextStates[e].position,reg.circleColliders[e].radius);
					auto& rb = reg.rigidPhysicsComponents[e];
					if (rb.affectedByGravity && !rb.isStatic) {
						nextStates[e].velocity.y += 9.81f*rb.gravityScale* dt;
					}
				}
			}
		}

		int totalCells = grid.cells.size();
		int batchSize = (totalCells + pool.GetWorkerCount() - 1) / pool.GetWorkerCount();

		std::atomic<int> tasksRemaining(0);

		for (int i = 0; i < totalCells; i += batchSize) {
			int end = std::min(i + batchSize, totalCells);
			tasksRemaining++;
			pool.Enqueue([this, i, end, &reg, &grid,&tasksRemaining]() {
				for (int c = i; c < end; c++) {
					auto& cell = grid.cells[c];
					if (cell.entities.size() < 2) continue;

					for (size_t a_idx = 0; a_idx < cell.entities.size(); a_idx++) {
						for (size_t b_idx = a_idx + 1; b_idx < cell.entities.size(); b_idx++) {
							Entity a = cell.entities[a_idx];
							Entity b = cell.entities[b_idx];
							std::lock_guard<std::mutex> lock(commandMutex);
							if (reg.HasComponent(a, COMP_CIRCLECOLLIDER) && reg.HasComponent(b, COMP_CIRCLECOLLIDER)) {
								if (CheckCircleCollision(a, b, reg)) {
									reg.circleColliders[a].isColliding = true;
									reg.circleColliders[b].isColliding = true;
								}
							}
						}
					}
				}
				tasksRemaining--;
				});
		}

		while (tasksRemaining > 0) {
			std::this_thread::yield();
		}
	}
};


