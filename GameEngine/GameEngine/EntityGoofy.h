#pragma once
#include "Entity.h"

class EntityGoofy : public Entity
{
private:


public:
	EntityGoofy() = default;
	~EntityGoofy() override;

	bool Init() override;

	//bool Update();

};

