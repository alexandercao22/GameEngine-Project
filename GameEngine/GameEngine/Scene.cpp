#include "Scene.h"

Scene::~Scene()
{
	if (_pool) {
		for (Entity *ent : _entities) {
			ent->~Entity();
			_pool->Free(ent);
		}
		delete _pool;
	}
	if (_buddy) {
		for (Entity *ent : _entities) {
			ent->~Entity();
			_buddy->Free(ent);
		}
		delete _buddy;
	}
	if (_stack) {
		for (Entity *ent : _entities) {
			ent->~Entity();
			_stack->Free();
		}
		delete _stack;
	}
}

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
	if (_pool) {
		for (Entity *ent : _entities) {
			ent->~Entity();
			_pool->Free(ent);
		}
	}
	if (_buddy) {
		for (Entity *ent : _entities) {
			ent->~Entity();
			_buddy->Free(ent);
		}
	}
	if (_stack) {
		for (Entity *ent : _entities) {
			ent->~Entity();
			_stack->Free();
		}
	}
	_entities.clear();
}

PoolAllocator *Scene::GetPoolAllocator()
{
	if (!_pool && !_buddy && !_stack) { // There can only be one allocator per ScenePart
		_pool = new PoolAllocator;
	}
	return _pool;
}

BuddyAllocator *Scene::GetBuddyAllocator()
{
	if (!_pool && !_buddy && !_stack) { // There can only be one allocator per ScenePart
		_buddy = new BuddyAllocator;
	}
	return _buddy;
}

StackAllocator *Scene::GetStackAllocator()
{
	if (!_pool && !_buddy && !_stack) { // There can only be one allocator per ScenePart
		_stack = new StackAllocator;
	}
	return _stack;
}
