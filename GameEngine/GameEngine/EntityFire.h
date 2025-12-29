#pragma once
#include "Entity.h"

class EntityFire : public Entity
{
private:
	float _health = 100.0f;
	Mesh _model;

public:
	EntityFire() = default;
	~EntityFire() override;

	bool Init() override;

	bool Update();
	Mesh GetLocalMesh();
};

