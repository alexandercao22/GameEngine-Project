#include "Entity.h"

bool Entity::Init()
{
	return true;
}

bool Entity::Update()
{
	return true;
}

Transform *Entity::GetTransform()
{
	return &_transform;
}

MeshResource *Entity::GetMesh()
{
	return _mesh;
}

TextureResource *Entity::GetTexture()
{
	return _texture;
}
