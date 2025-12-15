#include "ScenePart.h"
#include <math.h>

bool ScenePart::Init(Vector3 pos, std::string path) {
	_centerPos.x = pos.x;
	_centerPos.y = pos.y;
	_centerPos.z = pos.z;
	_pathToPackage = path;

	return true;
}

bool ScenePart::CheckDistance(Vector3 camera) {
	if (abs(_centerPos.x - camera.x) < 20 && abs(_centerPos.z - camera.z) < 20) {
		return true;
	}
	return false;
}

bool ScenePart::IsLoaded() {
	return _loaded.load();
}

void ScenePart::SetLoaded(bool val) {
	_loaded.store(val);
}

std::string ScenePart::GetPath() {
	SetLoaded(true);
	return _pathToPackage;
}

void ScenePart::AddEntity(Entity *entity)
{
	_entities.push_back(entity);
}

std::vector<Entity *> ScenePart::GetEntities()
{
	return _entities;
}

void ScenePart::DestroyEntities()
{
	for (Entity *ent : _entities) {
		delete ent;
	}
	_entities.clear();
}
