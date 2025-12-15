#pragma once

#include "Entity.h"
#include <vector>
#include <string>
#include "raylib.h"
#include "ScenePart.h"

class Scene
{
private:
	std::vector<std::string> _GUIDs; // GUIDs needed for this specific scene

	std::vector<Entity *> _entities;

	Model _floor;
	
	// The parts hold a package to a "lvl" and a distance. When distance is appropiate run async loading
	std::vector<ScenePart*> _parts;

	Camera3D _camera = { 0 };
	bool _showCursor = true;
	unsigned int _width = 1280;
	unsigned int _height = 720;

	bool RenderInterface();
	void RenderResources(Entity *ent);

	void Testing();

public:
	Scene() = default;
	~Scene();

	bool Init(unsigned int width = 1280, unsigned int height = 720);
	bool Update();
	bool RenderUpdate();
};

