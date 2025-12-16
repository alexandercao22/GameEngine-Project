#include "Scene.h"
#include <math.h>

bool Scene::Init(Vector3 pos, std::string path) {
	_centerPos.x = pos.x;
	_centerPos.y = pos.y;
	_centerPos.z = pos.z;
	_pathToPackage = path;

	return true;
}

bool Scene::CheckDistance(Vector3 camera) {
	if (abs(_centerPos.x - camera.x) < 20 && abs(_centerPos.z - camera.z) < 20) {
		return true;
	}
	return false;
}

bool Scene::IsLoaded() {
	return _loaded.load();
}

void Scene::SetLoaded(bool val) {
	_loaded.store(val);
}

std::string Scene::GetPath() {
	SetLoaded(true);
	return _pathToPackage;
}

void Scene::AddEntity(Entity *entity)
{
	_entities.push_back(entity);
}

std::vector<Entity *> Scene::GetEntities()
{
	return _entities;
}

void Scene::DestroyEntities()
{
	for (Entity *ent : _entities) {
		delete ent;
	}
	_entities.clear();
}
