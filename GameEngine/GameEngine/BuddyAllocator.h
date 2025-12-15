#pragma once

#include "MemoryTracker.h"
#include "Settings.h"

struct Buddy {
	unsigned int size = 0;
	int state = 0; // 0 = Free, 1 - Used, 2 - Split
	void *ptr = nullptr;
};

class BuddyAllocator
{
private:
	int _id = -1; // Allocator id (-1 = uninitialized)
	static int _nextId;

	unsigned int _size = 0;
	unsigned int _usedMemory = 0;
	void *_memory = nullptr;
	const int _maxDepthSize = 32;

	Buddy *_buddies = nullptr;
	int _numBuddies = 0;

public:
	BuddyAllocator() = default;
	~BuddyAllocator();

	int GetId() {
		return _id;
	}

	bool Init(unsigned int size = 1024);
	void *Request(unsigned int size, std::string tag = "No tag");
	bool Free(void *element);

	// Returns the current stats for the allocator
	BuddyStats GetStats();

	// Returns address of the allocators memory
	void *GetAddress();
	// Prints the state of every possible buddy
	void PrintStates();
	void DrawInterface();
};

// Buddy Allocator:
// https://bitsquid.blogspot.com/2015/08/allocation-adventures-3-buddy-allocator.html