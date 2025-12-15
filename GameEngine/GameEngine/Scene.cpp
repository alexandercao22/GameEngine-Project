#include "Scene.h"
#include "Settings.h"

#include "imgui.h"
#include "rlImGui.h"
#include "raymath.h"

#include "WinFileDialog.h"
#include <chrono>

#include "ResourceManager.h"
#include "MeshResource.h"
#include "TextureResource.h"

#include "EntityEnemy.h"
#include "EntityGoofy.h"
#include "EntityMushroom.h"

bool Scene::RenderInterface()
{
	ImGui::Begin("ImGui");
	ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetWindowSize(ImVec2(200.0f, _height));

	if (ImGui::Button("Open File Explorer")) {
		std::string path = OpenFileDialog();
		if (path.find(".obj") != std::string::npos ||
			path.find(".gltf") != std::string::npos ||
			path.find(".png") != std::string::npos ||
			path.find(".jpg") != std::string::npos) {
			CopyFileToResources(path, "Resources");

			size_t backslash = path.find_last_of("\\") + 1;
			std::string file = "Resources/" + path.substr(backslash, path.length() - backslash);
			std::cout << file << std::endl;

		}
	}

	static char buf[64] = "";
	ImGui::InputText("File", buf, IM_ARRAYSIZE(buf));
	if (ImGui::Button("Load")) {
		std::string path = "Resource/" + std::string(buf);
	}
	ImGui::Separator();
	ImGui::Text("Camera Position:");
	ImGui::BulletText("X: %.2f", _camera.position.x);
	ImGui::BulletText("Y: %.2f", _camera.position.y);
	ImGui::BulletText("Z: %.2f", _camera.position.z);
	ImGui::Separator();

	std::vector<std::string> resources = ResourceManager::Instance().GetCachedResources();
	for (int i = 0; i < resources.size(); i++) {
		ImGui::Text("%s", resources[i].c_str());

	}
	
	ImGui::End();
	return true;
}

void Scene::RenderResources(Entity *ent)
{
	Transform *transform = ent->GetTransform();

	// Frustum culling (kind of)
	Vector3 camToEnt = Vector3Normalize(transform->translation - _camera.position);
	Vector3 camForward = Vector3Normalize(_camera.target - _camera.position);
	float dot = Vector3DotProduct(camToEnt, camForward);
	if (dot < cos((_camera.fovy / 2))) {
		return;
	}

	MeshResource *mesh = ent->GetMesh();
	TextureResource *texture = ent->GetTexture();
	if (mesh != nullptr) {
		if (texture != nullptr) {
			SetMaterialTexture(&mesh->GetModel().materials[0], MATERIAL_MAP_DIFFUSE, texture->GetTexture());
			Vector3 rotation = { transform->rotation.x, transform->rotation.y, transform->rotation.z };
			DrawModelEx(mesh->GetModel(), transform->translation, rotation, transform->rotation.w, transform->scale, WHITE);
		}
		else {
			Vector3 rotation = { transform->rotation.x, transform->rotation.y, transform->rotation.z };
			DrawModelEx(mesh->GetModel(), transform->translation, rotation, transform->rotation.w, transform->scale, RED);
		}
	}
}

void Scene::Testing()
{
	static bool doneTesting = false;
	if (doneTesting) {
		return;
	}

	static int nRuns = 0;
	const int nIterations = 10;
	static float t = 0.0f;
	static double avg = 0.0f;

	const auto firstTestEnt = _entities.begin();
	if (_parts.size() > 0) {
		if (_parts[0]->CheckDistance(_camera.position) && !_parts[0]->IsLoaded()) {
			int numEnemies = 100;
			ResourceManager::Instance().AddPackage(_parts[0]->GetPath());
			_parts[0]->SetLoaded(true);
#ifdef TEST
			numEnemies = 10000;
#endif
			const int numRow = 10;
			auto t0 = std::chrono::high_resolution_clock::now();
			for (int i = 0; i < numEnemies; i++) {
				EntityEnemy *ent = new EntityEnemy;
				ent->Init();
				Transform *t = ent->GetTransform();
				t->translation.x = (int)(i / numRow) * -5;
				t->translation.z = (i % numRow) * -5;
				_parts[0]->AddEntity(ent);
				//_entities.push_back(ent);
			}
			auto t1 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> duration = t1 - t0;
			avg += duration.count();

#ifdef DEBUG
			std::cout << "Time to load " << numEnemies << " EntityEnemy (BLUE): " << duration.count() << "s" << std::endl;
#endif
		}
		else if (!_parts[0]->CheckDistance(_camera.position) && _parts[0]->IsLoaded()) {
			_parts[0]->DestroyEntities();
			_parts[0]->SetLoaded(false);
		}
	}

	if (nRuns > nIterations) {
		doneTesting = true;

#ifdef DEBUG
		avg /= nIterations;
		std::cout << "Average loading time (BLUE) of " << nIterations << " iterations: " << avg << "s" << std::endl;
#endif
	}

	t += GetFrameTime();
	if (_parts[0]->CheckDistance(_camera.position)) {
		if (t > 5.0f) {
			t = 0.0f;
			_camera.position = { 0.0f, 10.0f, 0.0f };
			_camera.target = { 0.0f, 10.0f, -1.0f };
		}
	}
	else {
		if (t > 1.0f) {
			t = 0.0f;
			_camera.position = { -40.0f, 10.0f, 0.0f };
			_camera.target = { -40.0f, 10.0f, -1.0f };
			nRuns++;
		}
	}
}

Scene::~Scene()
{
	for (Entity *ent : _entities) {
		delete ent;
	}

	for (ScenePart* part : _parts) {
		delete part;
	}

	ResourceManager::Instance().GetPackageManager()->UnmountAllPackages();
}

bool Scene::Init(unsigned int width, unsigned int height)
{
	_width = width;
	_height = height;

	InitWindow(_width, _height, "Game Engine Assignment 3");
	//SetTargetFPS(60);
	rlImGuiSetup(true);

	Mesh floorMesh = GenMeshPlane(40, 40, 1, 1);
	_floor = LoadModelFromMesh(floorMesh);

	for (int i = 1; i < 4; i++) {
		std::string packagePath = "Resources/Level" + std::to_string(i);
		if (!ResourceManager::Instance().GetPackageManager()->Pack(packagePath, "Resources")) {
			std::cerr << "Scene::Init(): Could not pack package:" << packagePath << std::endl;;
		}

		packagePath += ".gepak";
		if (!ResourceManager::Instance().GetPackageManager()->MountPackage(packagePath)) {
			std::cerr << "Scene::Init(): Could not load package: " << packagePath << std::endl;
			return false;
		}
	}

	//Initialize the parts
	ScenePart* level1 = new ScenePart; // BLUE
	level1->Init({ -40, 0, 0 }, "Resources/Level1.gepak");
	_parts.push_back(level1);

	ScenePart* level2 = new ScenePart; // GREEN
	level2->Init({ 0, 0, -40 }, "Resources/Level2.gepak");
	_parts.push_back(level2);

	ScenePart* level3= new ScenePart; // RED
	level3->Init({ -40, 0, -40 }, "Resources/Level3.gepak");
	_parts.push_back(level3);

	// Initialize camera
	_camera.position = { 0.0f, 2.0f, 10.0f };
	_camera.target = { 0.0f, 0.0f, 0.0f };
	_camera.up = { 0.0f, 1.0f, 0.0f };
	_camera.fovy = 90.0f;
	_camera.projection = CAMERA_PERSPECTIVE;

	EntityGoofy *goofy = new EntityGoofy;
	goofy->Init();
	Transform *t = goofy->GetTransform();
	t->translation = { 0.0f, 0.0f, 100.0f };
	t->scale = { 50.0f, 50.0f, 50.0f };
	_entities.push_back(goofy);

	return true;
}

bool Scene::Update()
{
	/* 
		Check distance between parts
		if distance is appropiate add the work to the thread vector
		so it can see it has work to do.
		Add the data from the thread onto the the RM queue.

		Check if the RM queue is empty. If not, load the model/texture
		with the data. 
	*/

	if (IsKeyPressed(KEY_C)) {
		_showCursor = !_showCursor;
		if (_showCursor) {
			EnableCursor();
		}
		else {
			DisableCursor();
		}
	}

	int size = ResourceManager::Instance().GetThreadDataSize();
//#ifdef DEBUG
//	std::cout << "ThreadDataSize: " << size << std::endl;
//#endif
	//if ( size > 0) {
	//	// Load the model or texture from the data in ThreadData datastructure
	//	// All this complexity would not be necessary if we did not use Raylib
	//	Resource* res = new MeshResource;
	//	ResourceManager::Instance().LoadObject(res);
	//}

	//for (auto& part : _parts) {
	//	if (part->CheckDistance(_camera.position)) {
	//		if (!part->IsLoaded()) {
	//			// Load
	//			std::string path = part->GetPath();
	//			ResourceManager::Instance().AddPackage(path);
	//		}
	//	}
	//}

#ifndef TEST
	// BLUE part
	if (_parts.size() > 0 &&
		_parts[0]->CheckDistance(_camera.position) && !_parts[0]->IsLoaded()) {
		ResourceManager::Instance().AddPackage(_parts[0]->GetPath());
		int numEnemies = 100;
		const int numRow = 10;

		for (int i = 0; i < numEnemies; i++) {
			EntityEnemy *ent = new EntityEnemy;
			ent->Init();
			Transform *t = ent->GetTransform();
			t->translation.x = (int)(i / numRow) * -5;
			t->translation.z = (i % numRow) * -5;
			_parts[0]->AddEntity(ent);
		}
	}
	else if (!_parts[0]->CheckDistance(_camera.position) && _parts[0]->IsLoaded()) {
		_parts[0]->DestroyEntities();
		_parts[0]->SetLoaded(false);
	}

	// GREEN part
	if (_parts.size() > 1 &&
		_parts[1]->CheckDistance(_camera.position) && !_parts[1]->IsLoaded()) {
		ResourceManager::Instance().AddPackage(_parts[1]->GetPath());
		int numEnemies = 100;
		const int numRow = 10;

		for (int i = 0; i < numEnemies; i++) {
			EntityGoofy *ent = new EntityGoofy;
			ent->Init();
			Transform *t = ent->GetTransform();
			t->translation.x = (int)(i / numRow) * -20;
			t->translation.y = 10.0f;
			t->translation.z = (i % numRow) * -10;
			t->scale = { 10.0f, 10.0f, 10.0f };
			_parts[1]->AddEntity(ent);
		}
	}
	else if (!_parts[1]->CheckDistance(_camera.position) && _parts[1]->IsLoaded()) {
		_parts[1]->DestroyEntities();
		_parts[1]->SetLoaded(false);
	}

	// RED part
	if (_parts.size() > 2 &&
		_parts[2]->CheckDistance(_camera.position) && !_parts[2]->IsLoaded()) {
		ResourceManager::Instance().AddPackage(_parts[2]->GetPath());
		int numEnemies = 100;
		const int numRow = 10;

		for (int i = 0; i < numEnemies; i++) {
			EntityMushroom *ent = new EntityMushroom;
			ent->Init();
			Transform *t = ent->GetTransform();
			t->translation.x = (int)(i / numRow) * -2;
			t->translation.z = (i % numRow) * -2;
			_parts[2]->AddEntity(ent);
		}
	}
	else if (!_parts[2]->CheckDistance(_camera.position) && _parts[2]->IsLoaded()) {
		_parts[2]->DestroyEntities();
		_parts[2]->SetLoaded(false);
	}
#endif

#ifdef TEST
	Testing();
#endif

	if (!_showCursor) {
		UpdateCamera(&_camera, CAMERA_FREE);
	}

	return true;
}

bool Scene::RenderUpdate()
{
	BeginDrawing();
	ClearBackground(RAYWHITE);
	rlImGuiBegin();
	BeginMode3D(_camera);

	//if (!RenderInterface()) {
	//	return false;
	//}

	Color colors[4] = { RED, GREEN, BLUE, YELLOW };
	int j = 0;
	for (int i = 0; i < 2; i++) {
		for (int k = 0; k < 2; k++) {
			DrawModel(_floor, Vector3{ -40.0f + k * 40, 0, -40.0f + i * 40 }, 1.0f, colors[j]);
			j++;
		}
	}

	for (ScenePart *part : _parts) {
		std::vector<Entity *> entities = part->GetEntities();
		for (Entity *ent : entities) {
			RenderResources(ent);
		}
	}

	for (Entity *ent : _entities) {
		RenderResources(ent);
	}

	DrawGrid(40, 1.0f);

	EndMode3D();
	DrawFPS(0, 0);
	rlImGuiEnd();
	EndDrawing();

	return true;
}
