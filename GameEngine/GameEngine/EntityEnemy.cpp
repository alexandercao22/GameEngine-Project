#include "EntityEnemy.h"

#include "ResourceManager.h"
#include "Resource.h"
#include "MeshResource.h"
#include "TextureResource.h"

EntityEnemy::~EntityEnemy()
{
    ResourceManager::Instance().UnloadResource("5f1e388c-39c4-471d-bfa2-727ab986dd1c");
    ResourceManager::Instance().UnloadResource("4fee39f8-43fc-46ba-9263-2081981e4637");
}

bool EntityEnemy::Init()
{
    if (_mesh || _texture) {
        std::cerr << "EntityEnemey::Init(): Enemy is already initialized" << std::endl;
        return false;
    }

    Resource *mesh = new MeshResource;
    if (!ResourceManager::Instance().LoadResource("5f1e388c-39c4-471d-bfa2-727ab986dd1c", mesh)) {
        std::cerr << "EntityEnemy::Init(): Failed to load mesh" << std::endl;
        delete mesh;
        return false;
    }
    _mesh = (MeshResource *)mesh;

    Resource *texture = new TextureResource;
    if (!ResourceManager::Instance().LoadResource("4fee39f8-43fc-46ba-9263-2081981e4637", texture)) {
        std::cerr << "EntityEnemy::Init(): Failed to load texture" << std::endl;
        delete texture;
    }
    else {
        _texture = (TextureResource *)texture;
    }

    return true;
}

bool EntityEnemy::Update()
{
    // Do something... like update _health
    return true;
}
