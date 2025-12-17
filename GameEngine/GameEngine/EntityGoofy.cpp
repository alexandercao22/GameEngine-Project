#include "EntityGoofy.h"

#include <iostream>

#include "ResourceManager.h"

EntityGoofy::~EntityGoofy()
{
    ResourceManager::Instance().UnloadResource("9da063c3-6b94-4df3-859e-6f23319b13e8");
    ResourceManager::Instance().UnloadResource("bd692322-f24c-41a4-a68e-bd31d954ac02");
}

bool EntityGoofy::Init()
{
    if (_mesh || _texture) {
        std::cerr << "EntityEnemey::Init(): Enemy is already initialized" << std::endl;
        return false;
    }

    Resource *mesh = new MeshResource;
    if (!ResourceManager::Instance().LoadResource("9da063c3-6b94-4df3-859e-6f23319b13e8", mesh)) {
        std::cerr << "EntityEnemy::Init(): Failed to load mesh" << std::endl;
        delete mesh;
        return false;
    }
    _mesh = (MeshResource *)mesh;

    Resource *texture = new TextureResource;
    if (!ResourceManager::Instance().LoadResource("bd692322-f24c-41a4-a68e-bd31d954ac02", texture)) {
        std::cerr << "EntityEnemy::Init(): Failed to load texture" << std::endl;
        delete texture;
    }
    else {
        _texture = (TextureResource *)texture;
    }

    return true;
}
