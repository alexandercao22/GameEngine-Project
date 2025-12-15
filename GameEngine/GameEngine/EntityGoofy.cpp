#include "EntityGoofy.h"

#include <iostream>

#include "ResourceManager.h"

EntityGoofy::~EntityGoofy()
{
    ResourceManager::Instance().UnloadResource("2ed278c8-3fd8-48fc-9ed9-72258ce32ddc");
    ResourceManager::Instance().UnloadResource("0c90b967-b4af-40fe-a0ab-719e95ffd424");
}

bool EntityGoofy::Init()
{
    if (_mesh || _texture) {
        std::cerr << "EntityEnemey::Init(): Enemy is already initialized" << std::endl;
        return false;
    }

    Resource *mesh = new MeshResource;
    if (!ResourceManager::Instance().LoadResource("2ed278c8-3fd8-48fc-9ed9-72258ce32ddc", mesh)) {
        std::cerr << "EntityEnemy::Init(): Failed to load mesh" << std::endl;
        delete mesh;
        return false;
    }
    _mesh = (MeshResource *)mesh;

    Resource *texture = new TextureResource;
    if (!ResourceManager::Instance().LoadResource("0c90b967-b4af-40fe-a0ab-719e95ffd424", texture)) {
        std::cerr << "EntityEnemy::Init(): Failed to load texture" << std::endl;
        delete texture;
    }
    else {
        _texture = (TextureResource *)texture;
    }

    return true;
}
