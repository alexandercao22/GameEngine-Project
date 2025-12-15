#pragma once

#include "Resource.h"
#include "MeshResource.h"
#include "TextureResource.h"

#include <string>
#include <vector>
#include "raylib.h"

class Entity
{
private:
	Transform _transform = {
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 0.0f },
		{ 1.0f, 1.0f, 1.0f }
	};

protected:
	MeshResource *_mesh = nullptr;
	TextureResource *_texture = nullptr;

public:
	Entity() = default;
	virtual ~Entity() = default;

	virtual bool Init();
	virtual bool Update();

	Transform *GetTransform();
	MeshResource *GetMesh();
	TextureResource *GetTexture();
};

