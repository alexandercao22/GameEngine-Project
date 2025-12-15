#include "ScenePart.h"
#include <math.h>

ScenePart::~ScenePart()
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

PoolAllocator *ScenePart::GetPoolAllocator()
{
	if (!_pool && !_buddy && !_stack) { // There can only be one allocator per ScenePart
		_pool = new PoolAllocator;
	}
	return _pool;
}

BuddyAllocator *ScenePart::GetBuddyAllocator()
{
	if (!_pool && !_buddy && !_stack) { // There can only be one allocator per ScenePart
		_buddy = new BuddyAllocator;
	}
	return _buddy;
}

StackAllocator *ScenePart::GetStackAllocator()
{
	if (!_pool && !_buddy && !_stack) { // There can only be one allocator per ScenePart
		_stack = new StackAllocator;
	}
	return _stack;
}
