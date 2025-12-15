#include "Interface.h"

#include "MemoryTracker.h"

#include <ctime>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include "raylib.h"
#include "imgui.h"
#include "rlImGui.h"

void Interface::ShowPoolInfo()
{
	std::unordered_map<int, PoolStats> poolAllocators = MemoryTracker::Instance().GetPoolAllocators();
	std::vector<std::string> poolIds;
	for (auto &pair : poolAllocators) {
		poolIds.push_back(std::to_string(pair.first));
	}
	static int currentPool = 0; // Current pool ID
	ImGui::Combo("PoolAllocators", &currentPool, [](void *data, int idx, const char **out_text)
		{
			auto &vec = *static_cast<std::vector<std::string>*>(data);
			if (idx < 0 || idx >= vec.size()) return false;
			*out_text = vec[idx].c_str();
			return true;
		}, &poolIds, poolIds.size());

	static float poolPercent = 0.0f;

	ImGui::NewLine();
	ImGui::Text("Allocation to current allocator");

	// Allocate
	const int nPools = poolAllocators.size();
	if (ImGui::Button("Allocate") && nPools > 0) {
		for (int i = 0; i < nPools; i++) {
			PoolContainer *currPool = &_pools[i];

			if (currentPool == currPool->pool->GetId()) {
				void *ptr = currPool->pool->Request();
				if (ptr) {
					currPool->ptrs.push_back(ptr);
				}

				// Update tracker
				PoolStats poolStats = currPool->pool->GetStats();
				MemoryTracker::Instance().TrackAllocator(currPool->pool->GetId(), poolStats);
				poolPercent = (float)poolStats.usedMemory / poolStats.capacity;

				break;
			}
		}
	}

	// Free
	// There needs to be at least 1 active pool allocator
	if (ImGui::Button("Free random") && nPools > 0) {
		for (int i = 0; i < nPools; i++) {
			PoolContainer *currPool = &_pools[i];

			if (currentPool == currPool->pool->GetId() && currPool->ptrs.size() > 0) {
				int randIdx = std::rand() % currPool->ptrs.size();
				if (currPool->pool->Free(currPool->ptrs[randIdx])) {
					currPool->ptrs.erase(currPool->ptrs.begin() + randIdx);
				}

				// Update tracker
				PoolStats poolStats = currPool->pool->GetStats();
				MemoryTracker::Instance().TrackAllocator(currPool->pool->GetId(), poolStats);
				poolPercent = (float)poolStats.usedMemory / poolStats.capacity;

				break;
			}
		}
	}

	// Info
	for (int i = 0; i < nPools; i++) {
		PoolContainer *currPool = &_pools[i];

		if (currentPool == currPool->pool->GetId()) {
			PoolStats poolStats = currPool->pool->GetStats();
			ImGui::Text(("Bytes used: " + std::to_string(poolStats.usedMemory) + "/" + std::to_string(poolStats.capacity)).c_str());
			break;
		}
	}

	ImGui::ProgressBar(poolPercent, ImVec2(0.0f, 0.0f));
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::Text("Used");

	for (int i = 0; i < nPools; i++) {
		PoolContainer *currPool = &_pools[i];

		if (currentPool == currPool->pool->GetId()) {
			ImDrawList *draw = ImGui::GetWindowDrawList();
			ImVec2 p = ImGui::GetCursorScreenPos();

			int poolNum = currPool->pool->GetNumSlots();
			float blockWidth = 40.0f;
			float blockHeight = 20.0f;
			for (int j = 0; j < poolNum; j++) {
				bool used = currPool->pool->GetUsed(j);
				ImU32 col = used ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 255, 0, 255);

				draw->AddRectFilled(ImVec2(p.x + j * blockWidth, p.y), ImVec2(p.x + (j + 1) * blockWidth - 1, p.y + blockHeight), col);
			}
			ImGui::NewLine();
			ImGui::NewLine();

			// Show all allocations to the current pool allocator
			std::unordered_map<void *, Allocation> allocations = MemoryTracker::Instance().GetAllocations();
			int index = 0;
			for (void *ptr : currPool->ptrs) {
				Allocation allocation = allocations[ptr];
				if (allocation.allocator == Allocator::Pool && allocation.allocatorId == currentPool) {
					ImGui::Text(std::string("Allocator ID: " + std::to_string(allocation.allocatorId)).c_str());
					const void *address = (const void *)allocation.ptr;
					std::stringstream ss;
					ss << address;
					ImGui::Text(std::string("Pointer: " + ss.str()).c_str());
					ImGui::Text(std::string("Size: " + std::to_string(allocation.size)).c_str());
					ImGui::Text(std::string("Tag: " + allocation.tag).c_str());
					ImGui::Text(std::string("Timestamp: " + FormatTimePoint(allocation.timestamp)).c_str());

					std::string buttonText = "Free##" + std::to_string(index++);
					if (ImGui::Button(buttonText.c_str())) {
						currPool->pool->Free(allocation.ptr);

						auto idx = std::find(currPool->ptrs.begin(), currPool->ptrs.end(), allocation.ptr);
						currPool->ptrs.erase(idx);

						// Update tracker
						PoolStats poolStats = currPool->pool->GetStats();
						MemoryTracker::Instance().TrackAllocator(currPool->pool->GetId(), poolStats);
						poolPercent = (float)poolStats.usedMemory / poolStats.capacity;
					}
					ImGui::NewLine();
				}
			}

			break;
		}
	}
}

void Interface::ShowBuddyInfo()
{
	std::unordered_map<int, BuddyStats> buddyAllocators = MemoryTracker::Instance().GetBuddyAllocators();
	std::vector<std::string> buddyIds;
	for (auto &pair : buddyAllocators) {
		buddyIds.push_back(std::to_string(pair.first));
	}
	static int currentBuddy = 0;
	ImGui::Combo("BuddyAllocators", &currentBuddy, [](void *data, int idx, const char **out_text)
		{
			auto &vec = *static_cast<std::vector<std::string>*>(data);
			if (idx < 0 || idx >= vec.size()) return false;
			*out_text = vec[idx].c_str();
			return true;
		}, &buddyIds, buddyIds.size());

	static float buddyPercent = 0.0f;

	ImGui::NewLine();
	ImGui::Text("Allocation to current allocator");

	// Allocate
	static int size = 0;
	ImGui::InputInt("Size", &size);

	const int nBuddies = buddyAllocators.size();
	if (ImGui::Button("Allocate") && nBuddies > 0) {
		for (int i = 0; i < nBuddies; i++) {
			BuddyContainer *currBuddy = &_buddies[i];

			if (currentBuddy == currBuddy->buddy->GetId()) {
				void *ptr = currBuddy->buddy->Request(size);
				if (ptr) {
					currBuddy->ptrs.push_back(ptr);
				}

				// Update tracker
				BuddyStats buddyStats = currBuddy->buddy->GetStats();
				MemoryTracker::Instance().TrackAllocator(currBuddy->buddy->GetId(), buddyStats);
				buddyPercent = (float)buddyStats.usedMemory / buddyStats.capacity;

				break;
			}
		}
	}

	// Free
	// There needs to be at least 1 active buddy allocator
	if (ImGui::Button("Free random") && nBuddies > 0) {
		for (int i = 0; i < nBuddies; i++) {
			BuddyContainer *currBuddy = &_buddies[i];

			if (currentBuddy == currBuddy->buddy->GetId() && currBuddy->ptrs.size() > 0) {
				int randIdx = std::rand() % currBuddy->ptrs.size();
				if (currBuddy->buddy->Free(currBuddy->ptrs[randIdx])) {
					currBuddy->ptrs.erase(currBuddy->ptrs.begin() + randIdx);
				}

				// Update tracker
				BuddyStats buddyStats = currBuddy->buddy->GetStats();
				MemoryTracker::Instance().TrackAllocator(currBuddy->buddy->GetId(), buddyStats);
				buddyPercent = (float)buddyStats.usedMemory / buddyStats.capacity;

				break;
			}
		}
	}

	// Info
	for (int i = 0; i < nBuddies; i++) {
		BuddyContainer *currBuddy = &_buddies[i];

		if (currentBuddy == currBuddy->buddy->GetId()) {
			BuddyStats buddyStats = currBuddy->buddy->GetStats();
			ImGui::Text(("Bytes used: " + std::to_string(buddyStats.usedMemory) + "/" + std::to_string(buddyStats.capacity)).c_str());
			break;
		}
	}

	ImGui::ProgressBar(buddyPercent, ImVec2(0.0f, 0.0f));
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::Text("Used");

	for (int i = 0; i < nBuddies; i++) {
		BuddyContainer *currBuddy = &_buddies[i];

		if (currentBuddy == currBuddy->buddy->GetId()) {
			currBuddy->buddy->DrawInterface();
		}
		ImGui::NewLine();
		ImGui::NewLine();

		// Show all allocations to the current pool allocator
		std::unordered_map<void *, Allocation> allocations = MemoryTracker::Instance().GetAllocations();
		int index = 0;
		for (auto &pair : allocations) {
			Allocation allocation = pair.second;
			if (allocation.allocator == Allocator::Buddy && allocation.allocatorId == currentBuddy) {
				ImGui::Text(std::string("Allocator ID: " + std::to_string(allocation.allocatorId)).c_str());
				const void *address = (const void *)allocation.ptr;
				std::stringstream ss;
				ss << address;
				ImGui::Text(std::string("Pointer: " + ss.str()).c_str());
				ImGui::Text(std::string("Size: " + std::to_string(allocation.size)).c_str());
				ImGui::Text(std::string("Tag: " + allocation.tag).c_str());
				ImGui::Text(std::string("Timestamp: " + FormatTimePoint(allocation.timestamp)).c_str());

				std::string buttonText = "Free##" + std::to_string(index++);
				if (ImGui::Button(buttonText.c_str())) {
					currBuddy->buddy->Free(allocation.ptr);

					auto idx = std::find(currBuddy->ptrs.begin(), currBuddy->ptrs.end(), allocation.ptr);
					currBuddy->ptrs.erase(idx);

					// Update tracker
					BuddyStats buddyStats = currBuddy->buddy->GetStats();
					MemoryTracker::Instance().TrackAllocator(currBuddy->buddy->GetId(), buddyStats);
					buddyPercent = (float)buddyStats.usedMemory / buddyStats.capacity;
				}
				ImGui::NewLine();
			}
		}
	}
}

void Interface::ShowStackInfo()
{
	std::unordered_map<int, StackStats> stackAllocators = MemoryTracker::Instance().GetStackAllocators();
	std::vector<std::string> stackIds;
	for (auto &pair : stackAllocators) {
		stackIds.push_back(std::to_string(pair.first));
	}
	static int currentStack = 0;
	ImGui::Combo("StackAllocators", &currentStack, [](void *data, int idx, const char **out_text)
		{
			auto &vec = *static_cast<std::vector<std::string>*>(data);
			if (idx < 0 || idx >= vec.size()) return false;
			*out_text = vec[idx].c_str();
			return true;
		}, &stackIds, stackIds.size());

	static float stackPercent= 0.0f;

	ImGui::NewLine();
	ImGui::Text("Allocation to current allocator");

	// Allocate
	static int size = 0;
	ImGui::InputInt("Size", &size);

	const int nStacks = stackAllocators.size();
	if (ImGui::Button("Allocate") && nStacks > 0) {
		for (int i = 0; i < nStacks; i++) {
			StackContainer *currStack = &_stacks[i];

			if (currentStack == currStack->stack->GetId()) {
				void *ptr = currStack->stack->Request(size);
				if (ptr) {
					currStack->ptrs.push_back(ptr);
				}

				// Update tracker
				StackStats stackStats = currStack->stack->GetStats();
				MemoryTracker::Instance().TrackAllocator(currStack->stack->GetId(), stackStats);
				stackPercent = (float)stackStats.usedMemory / stackStats.capacity;

				break;
			}
		}
	}

	// Free
	// There needs to be at least 1 active stack allocator
	if (ImGui::Button("Free") && nStacks > 0) {
		for (int i = 0; i < nStacks; i++) {
			StackContainer *currStack = &_stacks[i];

			if (currentStack == currStack->stack->GetId() && currStack->ptrs.size() > 0) {
				int randIdx = std::rand() % currStack->ptrs.size();
				if (currStack->stack->Free()) {
					currStack->ptrs.erase(currStack->ptrs.begin() + randIdx);
				}

				// Update tracker
				StackStats stackStats = currStack->stack->GetStats();
				MemoryTracker::Instance().TrackAllocator(currStack->stack->GetId(), stackStats);
				stackPercent = (float)stackStats.usedMemory / stackStats.capacity;

				break;
			}
		}
	}

	// Info
	StackContainer *currStack = nullptr;
	for (int i = 0; i < nStacks; i++) {
		currStack = &_stacks[i];
		if (currentStack == currStack->stack->GetId()) {
			StackStats stackStats = currStack->stack->GetStats();
			ImGui::Text(("Bytes used: " + std::to_string(stackStats.usedMemory) + "/" + std::to_string(stackStats.capacity)).c_str());
			break;
		}
	}

	ImGui::ProgressBar(stackPercent, ImVec2(0.0f, 0.0f));
	ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::Text("Used");

	ImGui::NewLine();
	ImGui::NewLine();

	// Show all allocations to the current pool allocator
	std::unordered_map<void *, Allocation> allocations = MemoryTracker::Instance().GetAllocations();
	int index = 0;
	for (auto &pair : allocations) {
		Allocation allocation = pair.second;
		if (allocation.allocator == Allocator::Stack && allocation.allocatorId == currentStack) {
			ImGui::Text(std::string("Allocator ID: " + std::to_string(allocation.allocatorId)).c_str());
			const void *address = (const void *)allocation.ptr;
			std::stringstream ss;
			ss << address;
			ImGui::Text(std::string("Pointer: " + ss.str()).c_str());
			ImGui::Text(std::string("Size: " + std::to_string(allocation.size)).c_str());
			ImGui::Text(std::string("Tag: " + allocation.tag).c_str());
			ImGui::Text(std::string("Timestamp: " + FormatTimePoint(allocation.timestamp)).c_str());
			ImGui::NewLine();
		}
	}
}

std::string Interface::FormatTimePoint(const std::chrono::system_clock::time_point &tp)
{
	std::time_t timeStamp = std::chrono::system_clock::to_time_t(tp);
	std::tm localTime{};

	localtime_s(&localTime, &timeStamp); // thread-safe Windows version

	std::ostringstream oss;
	oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
	return oss.str();

}

Interface::Interface()
{
	Init();
}

Interface::~Interface()
{
	for (PoolContainer pCon : _pools) {
		delete pCon.pool;
	}
	for (BuddyContainer bCon : _buddies) {
		delete bCon.buddy;
	}
	for (StackContainer sCon : _stacks) {
		delete sCon.stack;
	}
}

bool Interface::Init()
{
	std::srand(std::time({}));

	InitWindow(_width, _height, "Game Engine Assignment 2");
	SetTargetFPS(60);

	rlImGuiSetup(true);

	return false;
}

void Interface::Update()
{
	BeginDrawing();
	ClearBackground(BLACK);

	rlImGuiBegin();

	ImGui::Begin(" ", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
	ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetWindowSize(ImVec2(_width, _height));

	// Create allocator
	ImGui::Text("Create new allocator");
	static int currentType = 0;
	static int size = 0;
	static int slots = 10;
	const char *type[] = { "PoolAllocator", "BuddyAllocator", "StackAllocator" };
	ImGui::Combo("New Allocator", &currentType, type, IM_ARRAYSIZE(type));

	// What is need to initialize the chosen allocator
	if (currentType != 0) {
		ImGui::InputInt("Allocator size", &size);
	}
	else {
		ImGui::InputInt("Slot size", &size);
		ImGui::InputInt("Num of slots", &slots);
	}

	if (ImGui::Button("Create allocator")) {
		if (currentType == 0) { // Pool
			PoolContainer pCon;
			pCon.pool = new PoolAllocator;
			if (pCon.pool->Init(slots, size)) {
				_pools.push_back(pCon);
			}
		}
		else if (currentType == 1) { // Buddy
			BuddyContainer bCon;
			bCon.buddy = new BuddyAllocator;
			if (bCon.buddy->Init(size)) {
				_buddies.push_back(bCon);
			}
			else {
				ImGui::Text("Error");
			}
		}
		else if (currentType == 2) { // Stack
			StackContainer sCon;
			sCon.stack = new StackAllocator;
			if (sCon.stack->Init(size)) {
				_stacks.push_back(sCon);
			}
		}
	}

	// Allocator info
	ImGui::NewLine();
	ImGui::Text("Allocator info");

	static int chosenType = 0;
	ImGui::Combo("Allocator types", &chosenType, type, IM_ARRAYSIZE(type));

	if (chosenType == 0) { // Pool
		ShowPoolInfo();
	}
	else if (chosenType == 1) { // Buddy
		ShowBuddyInfo();
	}
	else if (chosenType == 2) { // Stack
		ShowStackInfo();
	}

	ImGui::End();
	rlImGuiEnd();
	EndDrawing();
}
