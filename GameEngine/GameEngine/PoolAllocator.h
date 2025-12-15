#pragma once
#include "MemoryTracker.h"
#include "Settings.h"
#include <vector>
#include <string>

struct Node {
	bool free;	// True if part of the list, otherwise false
	int next;	// Index of next node in the linked list
};

struct Block {
	void* address = nullptr;
	Node* nodes = nullptr;	// Pool slots tracker
	int numUsed;	// Number of free nodes (used for memory tracking)
	int head ;	// Index of the first free slot (-1 means no empty slots)
};

class PoolAllocator
{
private:
	int _id = -1; // Allocator id (-1 = uninitialized)
	static int _nextId;

	std::vector<Block> _blocks;

	int _n = -1;	// Number of slots contained in a single block (-1 = uninitialized)
	int _size = -1;	// Size of the slots in the pool (-1 = uninitialized)

	bool _aligned = false;

	// Initializes a new, empty block
	bool InitBlock(Block *block);

	// Creates a new, empty block and adds it to _blocks
	bool Expand();

public:
	PoolAllocator() = default;
	~PoolAllocator();

	int GetId() {
		return _id;
	}

	// If aligned is true, initial memory is set on size % = 0
	bool Init(int n, int size, bool aligned = false);
	// Get the first free slot
	void *Request(std::string tag = "No tag");
	bool Free(void *ptr);

	// Memory tracking

	// Returns the current stats for the allocator
	PoolStats GetStats();
	bool GetUsed(int index);
	int GetNumSlots();

	// Debug functionality

	// Returns the address of the block at the given index
	void* GetAdress(size_t index);
};

