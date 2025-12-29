#include "EntityFire.h"

#include "ResourceManager.h"
#include "Resource.h"
#include "MeshResource.h"
#include "TextureResource.h"

EntityFire::~EntityFire()
{
    //ResourceManager::Instance().UnloadResource("5f1e388c-39c4-471d-bfa2-727ab986dd1c");
    //ResourceManager::Instance().UnloadResource("2bda232-f349-40d2-b349-d7d8f583eddb");
    ResourceManager::Instance().UnloadResource("3815bea7-b312-4ff0-8767-3e854802f552");
    ResourceManager::Instance().UnloadResource("643c4ec7-1b4b-4b36-aa07-8fc7c8304a6f");
}

bool EntityFire::Init()
{
    if (_mesh || _texture) {
        std::cerr << "EntityFire::Init(): Enemy is already initialized" << std::endl;
        return false;
    }

    Resource* mesh = new MeshResource;
    if (!ResourceManager::Instance().LoadResource("643c4ec7-1b4b-4b36-aa07-8fc7c8304a6f", mesh)) {
        std::cerr << "EntityEnemy::Init(): Failed to load mesh" << std::endl;
        delete mesh;
        return false;
    }
    _mesh = (MeshResource*)mesh;
    //Mesh cube = GenMeshCube(0.5f, 0.5f, 0.5f);
    //_model = cube;

    Resource* texture = new TextureResource;
    /*if (!ResourceManager::Instance().LoadResource("3815bea7-b312-4ff0-8767-3e854802f552", texture)) {
        std::cerr << "EntityFire::Init(): Failed to load texture" << std::endl;
        delete texture;
    }*/
    if (!ResourceManager::Instance().LoadResource("3815bea7-b312-4ff0-8767-3e854802f552", texture)) {
        std::cerr << "EntityFire::Init(): Failed to load texture" << std::endl;
        delete texture;
    }
    else {
        _texture = (TextureResource*)texture;
    }

    return true;
}

bool EntityFire::Update()
{
    // Do something... like update _health
    return true;
}

Mesh EntityFire::GetLocalMesh() {
    return _model;
}
