#include "StackAllocator.h"

int StackAllocator::_nextId = 0;

bool StackAllocator::Init(int size) {

	_size = size;
	_start = malloc(size);
	_head = _start;
	int N = size / 4;
	_blockSize = new int[N];
	if (!_head) {
		std::cerr << "StackAllocator::Initialize(): failed to allocate block" << std::endl;
		return false;
	}

	_id = _nextId;
	_nextId++;

	if (TRACK_MEMORY) {
		MemoryTracker::Instance().TrackAllocator(_id, GetStats());
	}
	return true;
}

StackAllocator::~StackAllocator() {
	free(_start);

	if (TRACK_MEMORY) {
		MemoryTracker::Instance().RemoveAllocator(_id, Allocator::Stack);
	}
	delete _blockSize;
}

// Copy a pointer to the start of the block and update head
void* StackAllocator::Request(int size, std::string tag) {

	void* block = _head;

	if (static_cast<char*>(_head) + size > static_cast<char*>(_start) + _size) {
		std::cerr << "StackAllocator::Request(): memory request exceeds stack capacity" << std::endl;
		return nullptr;
	}
	_head = static_cast<char*>(_head) + size;


	_index++;
	_blockSize[_index] = size;

	if (TRACK_MEMORY) {
		MemoryTracker::Instance().StartTracking(Allocator::Stack, _id, block, size, tag);
	}

	return block;
}

bool StackAllocator::Free() {
	int diff = _blockSize[_index];
	_head = static_cast<char*>(_head) - _blockSize[_index];
	_index--;

	if (TRACK_MEMORY) {
		MemoryTracker::Instance().StopTracking(_head);
	}

	return true;
}

StackStats StackAllocator::GetStats()
{
	StackStats stats;
	stats.capacity = _size;
	ptrdiff_t diff = static_cast<char*>(_head) - static_cast<char*>(_start);
	stats.usedMemory = static_cast<unsigned int>(diff);

	return stats;
}

bool StackAllocator::Reset() {
	_head = _start;
	while (_index != -1) {
		Free();
	}
	_index = -1;

	return true;
}
