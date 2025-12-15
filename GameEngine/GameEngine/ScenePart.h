#pragma once
#include <string>
#include <atomic>
#include <vector>
#include "raylib.h"

#include "Entity.h"

// This is a class that Scene will hold to demonstrate asynchronous loading

class ScenePart {
private:
	std::string _pathToPackage;
	Vector3 _centerPos = { 0,0,0 };
	std::atomic<bool> _loaded{ false };

	std::vector<Entity *> _entities;

public:
	bool Init(Vector3 pos, std::string path);
	bool CheckDistance(Vector3 camera);
	bool IsLoaded();
	void SetLoaded(bool val);
	std::string GetPath();

	void AddEntity(Entity *entity);
	std::vector<Entity *> GetEntities();
	void DestroyEntities();

};