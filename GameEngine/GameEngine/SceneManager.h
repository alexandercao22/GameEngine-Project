#pragma once

#include "Entity.h"
#include <vector>
#include <string>
#include "raylib.h"
#include "Scene.h"
#include "PoolAllocator.h"
#include "StackAllocator.h"
#include "BuddyAllocator.h"

class SceneManager
{
private:
	// Global entities
	BuddyAllocator *_buddy = new BuddyAllocator;
	std::vector<PoolAllocator*> _poolAllocators;
	std::vector<StackAllocator*> _stackAllocators;
	std::vector<BuddyAllocator*> _buddyAllocators;

	std::vector<Entity *> _entities;

	Model _floor;
	
	// The parts hold a package to a "lvl" and a distance. When distance is appropiate run async loading
	std::vector<Scene *> _scenes;

	Camera3D _camera = { 0 };
	bool _showCursor = true;
	unsigned int _width = 1280;
	unsigned int _height = 720;

	bool RenderInterface();
	void RenderResources(Entity *ent);

	void Testing();

public:
	SceneManager() = default;
	~SceneManager();

	bool Init(unsigned int width = 1280, unsigned int height = 720);
	bool Update();
	bool RenderUpdate();
};

