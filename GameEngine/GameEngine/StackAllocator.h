#pragma once
#include "Settings.h"
#include "MemoryTracker.h"
#include <malloc.h>
#include <iostream>

class StackAllocator {
	
private:
	int _id;
	static int _nextId;

	void* _start;
	void* _head;
	int* _blockSize;
	int _size;
	int _index = -1;

public:
	StackAllocator() = default;
	~StackAllocator();

	int GetId() {
		return _id;
	}

	// Allocate memory space for the stack (bytes)
	bool Init(int size);

	void* Request(int size, std::string tag="No tag");
	bool Free();

	// Returns the current stats for the allocator
	StackStats GetStats();
	bool Reset();
};