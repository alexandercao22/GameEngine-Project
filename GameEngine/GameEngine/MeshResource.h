#pragma once
#include "Resource.h"
#include "raylib.h"
#include <string>

class MeshResource : public Resource {
private: 
	Model _model;

public:
	bool LoadFromData(const char* data, size_t size, const std::string& fileExtension);
	uint64_t GetMemoryUsage();

	MeshResource* LoadFromDisk(std::string GUID) override;
	void Unload() override;
	Model GetModel();
};