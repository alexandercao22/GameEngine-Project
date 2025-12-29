#pragma once
#include <string>
#include <atomic>
#include <vector>
#include "raylib.h"

#include "Entity.h"
#include "PoolAllocator.h"
#include "BuddyAllocator.h"
#include "StackAllocator.h"

// This is a class that Scene will hold to demonstrate asynchronous loading

class Scene {
private:
	std::string _pathToPackage;
	Vector3 _centerPos = { 0,0,0 };
	std::atomic<bool> _loaded{ false };
	int _lastFrame = 0;

	std::vector<Entity *> _entities;

	PoolAllocator *_pool = nullptr;
	BuddyAllocator *_buddy = nullptr;
	StackAllocator *_stack = nullptr;

public:
	Scene() = default;
	~Scene();

	bool Init(Vector3 pos, std::string path);
	bool CheckDistance(Vector3 camera);
	bool IsLoaded();
	void SetLoaded(bool val);
	int CheckLastFrame();
	void SetLastFrame(int frame);
	std::string GetPath();

	void AddEntity(Entity *entity);
	std::vector<Entity *>& GetEntities();
	void DestroyEntities();

	PoolAllocator *GetPoolAllocator();
	BuddyAllocator *GetBuddyAllocator();
	StackAllocator *GetStackAllocator();
};