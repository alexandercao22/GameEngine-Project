#pragma once

#include <vector>

#include "PoolAllocator.h"
#include "BuddyAllocator.h"
#include "StackAllocator.h"

#include "Entity.h"

struct PoolContainer {
	PoolAllocator *pool;
	std::vector<Entity *> *ptrs;
};

struct BuddyContainer {
	BuddyAllocator *buddy;
	std::vector<Entity *> *ptrs;
};

struct StackContainer {
	StackAllocator *stack;
	std::vector<Entity *> *ptrs;
};

class Interface
{
private:
	const int _width = 700;
	const int _height = 700;

	std::vector<PoolContainer> _pools;
	std::vector<BuddyContainer> _buddies;
	std::vector<StackContainer> _stacks;

	void ShowPoolInfo();
	void ShowBuddyInfo();
	void ShowStackInfo();

	// Time to string formatting
	std::string FormatTimePoint(const std::chrono::system_clock::time_point &tp);

public:
	// Singleton instance
	static Interface &Instance() {
		static Interface instance;
		return instance;
	}
	// Copy prevention
	Interface(const Interface &) = delete;
	Interface &operator=(const Interface &) = delete;

	Interface() = default;
	~Interface();

	void Update();
	void RenderInterface();

	void AddAllocator(PoolAllocator *&allocator, std::vector<Entity *> *ptrs);
	void AddAllocator(BuddyAllocator *&allocator, std::vector<Entity *> *ptrs);
	void AddAllocator(StackAllocator *&allocator, std::vector<Entity *> *ptrs);
};

