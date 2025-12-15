#include "MeshResource.h"

#include <iostream>
#include "raylib.h"
#include "OBJ_Loader.h"

bool MeshResource::LoadFromData(const char* data, size_t size, const std::string& fileExtension)
{
	if (fileExtension != ".obj") {
		std::cerr << "MeshResource::LoadFromData(): Failed to load unsupported format" << std::endl;
		return false;
	}

	objl::Loader objLoader;

	// Parse obj data
	if (!objLoader.LoadData(std::string(data, size))) {
		std::cerr << "MeshResource::LoadResource(): Failed to load mesh from data" << std::endl;
		return false;
	}

	objl::Mesh objMesh = objLoader.LoadedMeshes[0];

	// Convert to raylib mesh data (does not support the use of indexed vertices)
	size_t vertexCount = objMesh.Vertices.size();
	float* positions = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
	float* normals = (float*)MemAlloc(vertexCount * 3 * sizeof(float));
	float* uvs = (float*)MemAlloc(vertexCount * 2 * sizeof(float));

	for (int i = 0; i < vertexCount; i++) {
		positions[i * 3] = objMesh.Vertices[i].Position.X;
		positions[i * 3 + 1] = objMesh.Vertices[i].Position.Y;
		positions[i * 3 + 2] = objMesh.Vertices[i].Position.Z;

		normals[i * 3] = objMesh.Vertices[i].Normal.X;
		normals[i * 3 + 1] = objMesh.Vertices[i].Normal.Y;
		normals[i * 3 + 2] = objMesh.Vertices[i].Normal.Z;

		uvs[i * 2] = objMesh.Vertices[i].TextureCoordinate.X;
		uvs[i * 2 + 1] = objMesh.Vertices[i].TextureCoordinate.Y;
	}

	Mesh mesh{};
	mesh.vertexCount = vertexCount;
	mesh.vertices = positions;
	mesh.normals = normals;
	mesh.texcoords = uvs;
	mesh.triangleCount = vertexCount / 3;
	UploadMesh(&mesh, true);

	_model = LoadModelFromMesh(mesh);
	if (!IsModelValid(_model)) {
		std::cerr << "MeshResource::Init(): Loaded model was invalid" << std::endl;
		return false;
	}

	// Calculate memory usage
	_memoryUsage += (vertexCount * 3 * sizeof(float));
	_memoryUsage += (vertexCount * 3 * sizeof(float));
	_memoryUsage += (vertexCount * 2 * sizeof(float));

	return true;
}

uint64_t MeshResource::GetMemoryUsage()
{
	return _memoryUsage;
}

MeshResource* MeshResource::LoadFromDisk(std::string path) {
	Resource::RefAdd();
	
	Model model = LoadModel(path.c_str());
	if (!IsModelValid(model)) {
		std::cerr << "MeshResource::LoadFromDisk(): Mesh was invalid: " << path << std::endl;
		return nullptr;
	}
	_model = model;
	return this;
}

Model MeshResource::GetModel() {
	return _model;
}

void MeshResource::Unload() {
	UnloadModel(_model);
}