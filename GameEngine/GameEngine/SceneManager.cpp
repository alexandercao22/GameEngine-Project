#include "SceneManager.h"
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
#include "EntityFire.h"

bool SceneManager::RenderInterface()
{
	ImGui::Begin("Game Engine Project");
	// Standard window setup
	ImGui::SetWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(_width / 3.0f, (float)_height), ImGuiCond_Once);

	// Resource manager visualization
	{
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "RESOURCE MANAGER");

		std::vector<ResourceData> resources = ResourceManager::Instance().GetCachedResourcesData();
		static int selectedIndex = -1;

		if (selectedIndex >= (int)resources.size()) {
			selectedIndex = -1;
		}

		const char* previewValue = (selectedIndex >= 0 && selectedIndex < resources.size())
			? resources[selectedIndex].guid.c_str()
			: "Select a Resource...";

		if (ImGui::BeginCombo("Resources", previewValue)) {
			for (int i = 0; i < resources.size(); ++i) {
				const bool isSelected = (selectedIndex == i);

				// Create the selectable item
				if (ImGui::Selectable(resources[i].guid.c_str(), isSelected)) {
					selectedIndex = i;
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation)
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (selectedIndex >= 0 && selectedIndex < resources.size()) {
			ResourceData& selectedResource = resources[selectedIndex];

			ImGui::Separator();
			ImGui::Text("Selected Resource Details:");
			ImGui::BulletText("Type: %s", selectedResource.type.c_str());
			ImGui::BulletText("GUID: %s", selectedResource.guid.c_str());
			ImGui::BulletText("Ref Count: %d", selectedResource.refCount);

			ImGui::BulletText("Memory Usage: %llu bytes (%.2f MB)",
				selectedResource.memoryUsage,
				selectedResource.memoryUsage / (1024.0f * 1024.0f));
		}

	}

	ImGui::Spacing();
	ImGui::Spacing();

	// Memory tracking visualization
	{
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "MEMORY TRACKER");

		MemoryTracker& tracker = MemoryTracker::Instance();
		auto allAllocations = tracker.GetAllocations();

		// --- HELPER LAMBDAS ---
		auto FormatBytes = [](size_t bytes) -> std::string {
			if (bytes < 1024) return std::to_string(bytes) + " B";
			float k = (float)bytes / 1024.0f;
			std::stringstream ss; ss << std::fixed << std::setprecision(2) << k << " KB";
			return ss.str();
			};

		auto RenderAllocationList = [&](Allocator type, int id) {
			ImGui::PushID((int)type * 1000 + id); // Scope per allocator instance

			if (ImGui::TreeNode("Live Allocations")) {
				bool foundAny = false;
				int i = 0;
				for (auto& [ptr, alloc] : allAllocations) {
					if (alloc.allocator == type && alloc.allocatorId == id) {
						foundAny = true;
						ImGui::PushID(i++);

						std::string tag = alloc.tag.empty() ? "Untitled" : alloc.tag;
						ImGui::TextColored(ImVec4(0.6f, 1.0f, 1.0f, 1.0f), "%s", tag.c_str());
						ImGui::SameLine();
						ImGui::TextDisabled("(%p)", ptr);

						ImGui::Indent();
						ImGui::Text("Size: %s", FormatBytes(alloc.size).c_str());

						std::time_t t = std::chrono::system_clock::to_time_t(alloc.timestamp);
						char timeBuf[26]; ctime_s(timeBuf, sizeof(timeBuf), &t);
						timeBuf[std::strlen(timeBuf) - 1] = '\0';
						ImGui::TextDisabled("Time: %s", timeBuf);

						ImGui::Unindent();
						ImGui::PopID();
						ImGui::Separator();
					}
				}
				if (!foundAny) ImGui::TextDisabled("No active allocations.");
				ImGui::TreePop();
			}
			ImGui::PopID();
			};

		if (ImGui::CollapsingHeader("Stack Allocators")) {
			ImGui::PushID("Stacks"); // Global Stack Scope
			for (auto* stack : _stackAllocators) {
				ImGui::PushID(stack->GetId()); // Instance Scope

				StackStats stats = stack->GetStats();
				float fraction = (stats.capacity > 0) ? (float)stats.usedMemory / (float)stats.capacity : 0.0f;

				ImGui::Text("Stack ID: %d", stack->GetId());
				char overlay[32];
				sprintf_s(overlay, "%.1f%% (%s / %s)", fraction * 100.0f, FormatBytes(stats.usedMemory).c_str(), FormatBytes(stats.capacity).c_str());
				ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), overlay);

				RenderAllocationList(Allocator::Stack, stack->GetId());

				ImGui::Separator();
				ImGui::PopID();
			}
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Pool Allocators")) {
			ImGui::PushID("Pools"); // Global Pool Scope
			for (auto* pool : _poolAllocators) {
				ImGui::PushID(pool->GetId()); // Instance Scope

				PoolStats stats = pool->GetStats();
				float fraction = (stats.capacity > 0) ? (float)stats.usedMemory / (float)stats.capacity : 0.0f;

				ImGui::Text("Pool ID: %d | Block Size: %s", pool->GetId(), FormatBytes(stats.capacity / (stats.numBlocks > 0 ? stats.numBlocks : 1)).c_str());

				char overlay[32];
				sprintf_s(overlay, "%.1f%%", fraction * 100.0f);
				ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), overlay);

				// --- VISUALIZATION BLOCK START ---
				ImGui::Text("Block Map:");
				ImDrawList* draw = ImGui::GetWindowDrawList();
				ImVec2 p = ImGui::GetCursorScreenPos();

				int numSlots = pool->GetNumSlots();
				float blockWidth = 12.0f;
				float blockHeight = 12.0f;
				float spacing = 2.0f;
				float xOffset = 0.0f;
				float yOffset = 0.0f;
				float windowWidth = ImGui::GetContentRegionAvail().x;

				for (int j = 0; j < numSlots; j++) {
					// Wrap logic
					if (xOffset + blockWidth > windowWidth) {
						xOffset = 0.0f;
						yOffset += blockHeight + spacing;
					}

					bool used = pool->GetUsed(j);
					ImU32 col = used ? IM_COL32(50, 220, 50, 255) : IM_COL32(220, 50, 50, 255);

					// Draw Rect
					draw->AddRectFilled(ImVec2(p.x + xOffset, p.y + yOffset), ImVec2(p.x + xOffset + blockWidth, p.y + yOffset + blockHeight), col);

					// Optional Tooltip
					if (ImGui::IsMouseHoveringRect(ImVec2(p.x + xOffset, p.y + yOffset), ImVec2(p.x + xOffset + blockWidth, p.y + yOffset + blockHeight))) {
						ImGui::BeginTooltip();
						ImGui::Text("Block %d: %s", j, used ? "Free" : "Used");
						ImGui::EndTooltip();
					}

					xOffset += blockWidth + spacing;
				}
				// Reserve layout space so text doesn't overlap blocks
				ImGui::Dummy(ImVec2(0.0f, yOffset + blockHeight + spacing));
				// --- VISUALIZATION BLOCK END ---

				RenderAllocationList(Allocator::Pool, pool->GetId());

				ImGui::Separator();
				ImGui::PopID();
			}
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Buddy Allocators")) {
			ImGui::PushID("Buddies"); // Global Buddy Scope
			for (auto* buddy : _buddyAllocators) {
				ImGui::PushID(buddy->GetId()); // Instance Scope

				BuddyStats stats = buddy->GetStats();
				float fraction = (stats.capacity > 0) ? (float)stats.usedMemory / (float)stats.capacity : 0.0f;

				ImGui::Text("Buddy ID: %d", buddy->GetId());

				char overlay[32];
				sprintf_s(overlay, "%.1f%% (%s / %s)", fraction * 100.0f, FormatBytes(stats.usedMemory).c_str(), FormatBytes(stats.capacity).c_str());
				ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), overlay);
				buddy->DrawInterface();

				RenderAllocationList(Allocator::Buddy, buddy->GetId());

				ImGui::Separator();
				ImGui::PopID();
			}
			ImGui::PopID();
		}
	}

	ImGui::End();
	return true;
}

void SceneManager::RenderResources(Entity *ent)
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

void SceneManager::Testing()
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
	if (_scenes.size() > 0) {
		if (_scenes[0]->CheckDistance(_camera.position) && !_scenes[0]->IsLoaded()) {
			int numEnemies = 100;
			ResourceManager::Instance().AddPackage(_scenes[0]->GetPath());
			_scenes[0]->SetLoaded(true);
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
				_scenes[0]->AddEntity(ent);
				//_entities.push_back(ent);
			}
			auto t1 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> duration = t1 - t0;
			avg += duration.count();

#ifdef DEBUG
			std::cout << "Time to load " << numEnemies << " EntityEnemy (BLUE): " << duration.count() << "s" << std::endl;
#endif
		}
		else if (!_scenes[0]->CheckDistance(_camera.position) && _scenes[0]->IsLoaded()) {
			_scenes[0]->DestroyEntities();
			_scenes[0]->SetLoaded(false);
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
	if (_scenes[0]->CheckDistance(_camera.position)) {
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

SceneManager::~SceneManager()
{
	for (Entity *ent : _entities) {
		ent->~Entity();
		_buddy->Free(ent);
	}

	for (Scene *scene : _scenes) {
		PoolAllocator *pool = scene->GetPoolAllocator();
		BuddyAllocator *buddy = scene->GetBuddyAllocator();
		StackAllocator *stack = scene->GetStackAllocator();

		if (pool)
			_poolAllocators.erase(std::find(_poolAllocators.begin(), _poolAllocators.end(), pool));
		if (buddy)
			_buddyAllocators.erase(std::find(_buddyAllocators.begin(), _buddyAllocators.end(), buddy));
		if (stack)
			_stackAllocators.erase(std::find(_stackAllocators.begin(), _stackAllocators.end(), stack));

		delete scene;
	}

	for (PoolAllocator* allocator : _poolAllocators) {
		delete allocator;
	}
	for (StackAllocator* allocator : _stackAllocators) {
		delete allocator;
	}
	for (BuddyAllocator* allocator : _buddyAllocators) {
		delete allocator;
	}

	ResourceManager::Instance().GetPackageManager()->UnmountAllPackages();
}

bool SceneManager::Init(unsigned int width, unsigned int height)
{

	_width = width;
	_height = height;

	InitWindow(_width, _height, "Game Engine Architecture Project");
	//SetTargetFPS(60);
	rlImGuiSetup(true);

	// Initialize the global buddy allocator
	_buddy->Init(512);
	_buddyAllocators.emplace_back(_buddy);

	// Intiialize floors
	Mesh floorMesh = GenMeshPlane(40, 40, 1, 1);
	_floor = LoadModelFromMesh(floorMesh);

	// Pack and mount packages
	for (int i = 1; i < 4; i++) {
		std::string packagePath = "Resources/Level" + std::to_string(i);
		if (!ResourceManager::Instance().GetPackageManager()->Pack(packagePath, "Resources")) {
			std::cerr << "SceneManager::Init(): Could not pack package:" << packagePath << std::endl;;
		}

		packagePath += ".gepak";
		if (!ResourceManager::Instance().GetPackageManager()->MountPackage(packagePath)) {
			std::cerr << "SceneManager::Init(): Could not load package: " << packagePath << std::endl;
			return false;
		}
	}

	//Initialize the parts
	{
		Scene *level1 = new Scene; // BLUE / POOL
		level1->Init({ -40, 0, 0 }, "Resources/Level1.gepak");
		PoolAllocator *lvlPool = level1->GetPoolAllocator();
		lvlPool->Init(20, sizeof(EntityEnemy));
		_poolAllocators.emplace_back(lvlPool);
		_scenes.push_back(level1);

		Scene *level2 = new Scene; // GREEN / BUDDY
		level2->Init({ 0, 0, -40 }, "Resources/Level2.gepak");
		BuddyAllocator *lvlBuddy = level2->GetBuddyAllocator();
		lvlBuddy->Init(std::pow(2, 12)); // 2048 Bytes
		_buddyAllocators.emplace_back(lvlBuddy);
		_scenes.push_back(level2);

		Scene *level3 = new Scene; // RED / STACK
		level3->Init({ -40, 0, -40 }, "Resources/Level3.gepak");
		StackAllocator *lvlStack = level3->GetStackAllocator();
		lvlStack->Init(105 * sizeof(EntityFire));
		_stackAllocators.emplace_back(lvlStack);
		_scenes.push_back(level3);
	}

	// Initialize camera
	{
		_camera.position = { 0.0f, 2.0f, 10.0f };
		_camera.target = { 0.0f, 0.0f, 0.0f };
		_camera.up = { 0.0f, 1.0f, 0.0f };
		_camera.fovy = 90.0f;
		_camera.projection = CAMERA_PERSPECTIVE;
	}

	// Fill pool allocator
	{
		//void *nme = (EntityEnemy *)_buddy.Request(sizeof(EntityEnemy));
		//EntityEnemy *ent = new (nme) EntityEnemy();
		//ent->Init();
		//_entities.push_back(ent);

		void *ahyuck = (EntityGoofy *)_buddy->Request(sizeof(EntityGoofy), "Goofy");
		EntityGoofy *goofy = new (ahyuck) EntityGoofy;
		goofy->Init();
		Transform *t = goofy->GetTransform();
		t->translation = { 0.0f, 0.0f, 100.0f };
		t->scale = { 50.0f, 50.0f, 50.0f };
		_entities.push_back(goofy);
	}

	return true;
}

bool SceneManager::Update()
{
	// Memory tracking updates (every 0.5s)
	static float elapsed = 0;
	elapsed += GetFrameTime();
	if (elapsed > 0.5) {
		elapsed -= 0.5f;
		for (auto& allocator : _poolAllocators) {
			MemoryTracker::Instance().TrackAllocator(allocator->GetId(), allocator->GetStats());
		}
		for (auto& allocator : _stackAllocators) {
			MemoryTracker::Instance().TrackAllocator(allocator->GetId(), allocator->GetStats());
		}
		for (auto& allocator : _buddyAllocators) {
			MemoryTracker::Instance().TrackAllocator(allocator->GetId(), allocator->GetStats());
		}
	}

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

	//for (auto& scene : _scenes) {
	//	if (scene->CheckDistance(_camera.position)) {
	//		if (!scene->IsLoaded()) {
	//			// Load
	//			std::string path = scene->GetPath();
	//			ResourceManager::Instance().AddPackage(path);
	//		}
	//	}
	//}

#ifndef TEST
	// BLUE / POOL
	static float deltaTime = 0;
	deltaTime += GetFrameTime();
	if (_scenes.size() > 0 &&
		_scenes[0]->CheckDistance(_camera.position) && !_scenes[0]->IsLoaded()) {
		ResourceManager::Instance().AddPackage(_scenes[0]->GetPath());
		int numEnemies = 5;
		const int numRow = 10;

		for (int i = 0; i < numEnemies; i++) {
			void *ptr = _scenes[0]->GetPoolAllocator()->Request();
			EntityEnemy *ent = new (ptr) EntityEnemy; // Cast the empty memory to an Entity
			if (!ptr) {
				break;
			}
			ent->Init();
			Transform *t = ent->GetTransform();
			t->translation.x = (int)(i / numRow) * -5;
			t->translation.z = (i % numRow) * -5;
			_scenes[0]->AddEntity(ent);
		}
	}
	else if (!_scenes[0]->CheckDistance(_camera.position) && _scenes[0]->IsLoaded()) {
		_scenes[0]->DestroyEntities();
		_scenes[0]->SetLoaded(false);
	}
	else if (_scenes[0]->CheckDistance(_camera.position) && _scenes[0]->IsLoaded() && deltaTime > 2) {
		deltaTime -= 2;
		std::vector<Entity*> &entities  = _scenes[0]->GetEntities();
		for (int i = entities.size() - 1; i > 0; i--) {
			int spawn = rand() % 2;
			if (spawn == 0) {
				Entity* ent = entities[i];           
				ent->~Entity();                       
				_scenes[0]->GetPoolAllocator()->Free(ent);  
				entities.erase(entities.begin() + i);
			}
		}
		for (int i = 0; i < 5; i++) {
			void* ptr = _scenes[0]->GetPoolAllocator()->Request();
			EntityEnemy* ent = new (ptr) EntityEnemy;
			if (!ptr) {
				break;
			}
			ent->Init();
			Transform* t = ent->GetTransform();
			float x = rand() % 20;
			float z = rand() % 20;
			t->translation.x = -(x + 20);
			t->translation.z = -(z + 20);
			_scenes[0]->AddEntity(ent);
		}
	}

	// GREEN / BUDDY
	if (_scenes.size() > 1 &&
		_scenes[1]->CheckDistance(_camera.position) && !_scenes[1]->IsLoaded()) {
		ResourceManager::Instance().AddPackage(_scenes[1]->GetPath());
		int numEnemies = 10;
		const int numRow = 10;

		for (int i = 0; i < numEnemies; i++) {
			void *ptr = nullptr;
			if (i % 3 == 0) {
				ptr = _scenes[1]->GetBuddyAllocator()->Request(sizeof(EntityGoofy));
				if (!ptr) {
					break;
				}
				EntityGoofy *ent = new (ptr) EntityGoofy; // Cast the empty memory to an Entity
				ent->Init();
				Transform *t = ent->GetTransform();
				t->translation.x = (int)(i / numRow) * -20;
				t->translation.y = 0.0f;
				t->translation.z = (i % numRow) * -10;
				t->scale = { 10.0f, 10.0f, 10.0f };
				_scenes[1]->AddEntity(ent);
			}
			else if (i % 3 == 1) {
				ptr = _scenes[1]->GetBuddyAllocator()->Request(sizeof(EntityEnemy));
				EntityEnemy *ent = new (ptr) EntityEnemy; // Cast the empty memory to an Entity
				if (!ptr) {
					break;
				}
				ent->Init();
				Transform *t = ent->GetTransform();
				t->translation.x = (int)(i / numRow) * -5;
				t->translation.z = (i % numRow) * -5;
				_scenes[1]->AddEntity(ent);
			}
			else if (i % 3 == 2) {
				ptr = _scenes[1]->GetBuddyAllocator()->Request(sizeof(EntityMushroom));
				if (!ptr) {
					break;
				}
				EntityMushroom *ent = new (ptr) EntityMushroom;
				ent->Init();
				Transform *t = ent->GetTransform();
				t->translation.x = (int)(i / numRow) * -2;
				t->translation.z = (i % numRow) * -2;
				_scenes[1]->AddEntity(ent);
			}
		}
	}
	else if (!_scenes[1]->CheckDistance(_camera.position) && _scenes[1]->IsLoaded()) {
		_scenes[1]->DestroyEntities();
		_scenes[1]->SetLoaded(false);
	}
	else if (_scenes[1]->CheckDistance(_camera.position) && _scenes[1]->IsLoaded() && deltaTime > 2) {
		deltaTime -= 2;
		std::vector<Entity*>& entities = _scenes[1]->GetEntities();
		for (int i = entities.size() - 1; i > 0; i--) {
			int spawn = rand() % 2;
			if (spawn == 0) {
				Entity* ent = entities[i];
				ent->~Entity();
				_scenes[1]->GetBuddyAllocator()->Free(ent);
				entities.erase(entities.begin() + i);
			}
		}
		for (int i = 0; i < 5; i++) {
			int unit = rand() % 3;
			void* ptr = nullptr;
			Entity* ent = nullptr;
			switch (unit) {
			case 0:
				ptr = _scenes[1]->GetBuddyAllocator()->Request(sizeof(EntityEnemy));
				if (!ptr) break;
				ent = new (ptr) EntityEnemy;
				//ent->Init();
				break;
			case 1:
				ptr = _scenes[1]->GetBuddyAllocator()->Request(sizeof(EntityGoofy));
				if (!ptr) break;
				 ent = new (ptr) EntityGoofy;
				//ent->Init();
				break;
			case 2:
				ptr = _scenes[1]->GetBuddyAllocator()->Request(sizeof(EntityMushroom));
				if (!ptr) break;
				 ent = new (ptr) EntityMushroom;
				//ent->Init();
				break;
			}

			if (!ent) {
				break;
			}
			ent->Init();
			Transform* t = ent->GetTransform();
			float x = rand() % 40;
			float z = rand() % 20;
			t->translation.x = -(x);
			t->translation.z = -(z + 40);
			_scenes[1]->AddEntity(ent);
		}
	}

	// RED / STACK
	// Has an extra check for loading since it reloads each frame
	if (_scenes.size() > 2 &&
		_scenes[2]->CheckDistance(_camera.position)) {
		if (!_scenes[2]->IsLoaded()) {
			ResourceManager::Instance().AddPackage(_scenes[2]->GetPath());

		}
		_scenes[2]->GetStackAllocator()->Reset();
		for (auto ent : _frameFireEntities) {
			ent->~EntityFire();
		}
		_frameFireEntities.clear();

		int numEnemies = 64;
		const int numRow = 10;
		std::vector<Middle> middlerow;
		Middle first = { 0,0 };
		middlerow.push_back(first);
		int middleTop = 1;
		
		// Lägg till eldklumpar istället
		float startPosX = -40;
		for (int i = 0; i < numEnemies; i++) {

			void *ptr = _scenes[2]->GetStackAllocator()->Request(sizeof(EntityFire));
			if (!ptr) {
				std::cout << "Error: StackAllocator" << std::endl;
			}
			EntityFire* ent = new (ptr) EntityFire;
			ent->Init();
			float offsetX = 0.0f;
			float offsetY = 0.0f;

			int random = rand() % middleTop;
			if (random == 0) {
				Middle middle;
				middlerow.push_back(middle);
				middleTop++;
			}
			else {

				int growth = rand() % 4;
				switch (growth) {
					case 0:
						middlerow[random - 1].left += 0.5;
						break;
					case 1:
						middlerow[random - 1].right += 0.5;
						break;
					case 2:
						middlerow[random - 1].forward += 0.5;
						break;
					case 3:
						middlerow[random - 1].backward += 0.5;
						break;
					default:
						break;
				}
				if (growth < 2) {
					offsetX = growth == 0 ? -middlerow[random - 1].left : middlerow[random - 1].right;
					int other = rand() % 2;
					offsetY = other == 0 ? -middlerow[random - 1].backward : middlerow[random - 1].forward;
				}
				else {
					offsetY = growth == 0 ? -middlerow[random - 1].backward : middlerow[random - 1].forward;
					int other = rand() % 2;
					offsetY = other == 0 ? -middlerow[random - 1].left : middlerow[random - 1].right;
				}


			}
			Transform* t = ent->GetTransform();
			t->translation.x = startPosX + offsetX;
			t->translation.y = random * 0.5f + 0.5;
			t->translation.z = startPosX + offsetY;
		
			_frameFireEntities.push_back(ent);
		}

		_scenes[2]->SetLoaded(true);
	}
	else if ((!_scenes[2]->CheckDistance(_camera.position) && _scenes[2]->IsLoaded())) {
		for (auto ent : _frameFireEntities) {
			ent->~EntityFire();
		}
		_frameFireEntities.clear();
		_scenes[2]->SetLoaded(false);
	}
	
#endif

#ifdef TEST
	Testing();
#endif

	if (!_showCursor) {
		UpdateCamera(&_camera, CAMERA_FREE);
		SetMousePosition(_width / 2, _height / 2);
	}

	

	return true;
}

bool SceneManager::RenderUpdate()
{
	BeginDrawing();
	ClearBackground(RAYWHITE);
	rlImGuiBegin();
	BeginMode3D(_camera);

	if (!RenderInterface()) {
		return false;
	}

	Color colors[4] = { RED, GREEN, BLUE, YELLOW };
	int j = 0;
	for (int i = 0; i < 2; i++) {
		for (int k = 0; k < 2; k++) {
			DrawModel(_floor, Vector3{ -40.0f + k * 40, 0, -40.0f + i * 40 }, 1.0f, colors[j]);
			j++;
		}
	}

	for (Scene *scene : _scenes) {
		
		std::vector<Entity *> entities = scene->GetEntities();
		for (Entity *ent : entities) {
			RenderResources(ent);
		}
		
	}
	for (EntityFire* fire : _frameFireEntities) {
		RenderResources(fire);
	}

	for (Entity *ent : _entities) {
		RenderResources(ent);
	}


	DrawGrid(40, 1.0f);

	EndMode3D();
	DrawFPS(GetScreenWidth() - 50, 0);
	rlImGuiEnd();
	EndDrawing();

	return true;
}
