#include "EntityMushroom.h"

#include <iostream>

#include "ResourceManager.h"

EntityMushroom::~EntityMushroom()
{
    ResourceManager::Instance().UnloadResource("bcc3669b-21be-412a-a6f6-7a0d863d51df");
}

bool EntityMushroom::Init()
{
    if (_mesh || _texture) {
        std::cerr << "EntityEnemey::Init(): Enemy is already initialized" << std::endl;
        return false;
    }

    Resource *mesh = new MeshResource;
    if (!ResourceManager::Instance().LoadResource("bcc3669b-21be-412a-a6f6-7a0d863d51df", mesh)) {
        std::cerr << "EntityEnemy::Init(): Failed to load mesh" << std::endl;
        delete mesh;
        return false;
    }
    _mesh = (MeshResource *)mesh;

    return true;
}
