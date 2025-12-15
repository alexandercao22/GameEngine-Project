#include "PoolAllocator.h"
#include "Settings.h"
#include <malloc.h>
#include <iostream>

int PoolAllocator::_nextId = 0; // Set the initial id

bool PoolAllocator::InitBlock(Block *block)
{
	if (_aligned) {
		block->address = _aligned_malloc(static_cast<size_t>(_n * _size), _size);
	}
	else {
		block->address = malloc(static_cast<size_t>(_n * _size));
	}
	if (!block->address) {
		std::cerr << "PoolAllocator::InitBlock(): failed to allocate pool block" << std::endl;
		return false;
	}

	block->nodes = (Node*)malloc(static_cast<size_t>(_n) * sizeof(Node));
	if (!block->nodes) {
		std::cerr << "PoolAllocator::InitBlock(): failed to allocate nodes" << std::endl;
		free(block->address);
		block->address = nullptr;
		return false;
	}

	block->numUsed = 0;
	block->head = 0;
	for (int i = 0; i < _n; i++) {
		block->nodes[i].free = true;
		if (i == _n - 1) block->nodes[i].next = -1;
		else block->nodes[i].next = i + 1;
	}

	return true;
}

bool PoolAllocator::Expand()
{
	Block newBlock;
	if (!InitBlock(&newBlock)) {
		std::cerr << "PoolAllocator::Expand(): Failed creating new pool block" << std::endl;
		return false;
	}

	_blocks.push_back(newBlock);

	return true;
}

PoolStats PoolAllocator::GetStats()
{
	PoolStats stats;
	stats.capacity = _n * _size * _blocks.size();
	stats.numBlocks = _blocks.size();
	stats.usedMemory = 0;

	for (Block& block : _blocks) {
		stats.usedMemory += block.numUsed * _size;
	}

	return stats;
}

PoolAllocator::~PoolAllocator()
{
	for (Block& block : _blocks) {
		if (_aligned) {
			_aligned_free(block.address);
		}
		else {
			free(block.address);
		}
		block.address = nullptr;

		free(block.nodes);
		block.nodes = nullptr;
	}

	if (TRACK_MEMORY) {
		MemoryTracker::Instance().RemoveAllocator(_id, Allocator::Pool);
	}
}

bool PoolAllocator::Init(int n, int size, bool aligned)
{
	_n = n;
	_size = size;
	_aligned = aligned;

	Block block;
	if (!InitBlock(&block)) {
		std::cerr << "PoolAllocator::Init(): Failed creating initial pool block" << std::endl;
		// Reset member values to indicate that the Allocator is still uninitialized
		_n = -1;
		_size = -1;
		return false;
	}

	_blocks.push_back(block);

	_id = _nextId;
	_nextId++;

	if (TRACK_MEMORY) {
		MemoryTracker::Instance().TrackAllocator(_id, GetStats());
	}

	return true;
}

void *PoolAllocator::Request(std::string tag)
{
	// Should use Expand() to create a new block if all current blocks are full
	// Additionally, new allocations should be placed in the first block with empty slots :)

	for (int i = 0; i < _blocks.size(); i++) {
		
		Block& block = _blocks[i];

		// Get index of first free node
		int index = block.head;

		// Check if list was full
		// Add new block and jump to next iteration of blocks
		if (index == -1) {
			
			if (i == _blocks.size() - 1) {
				if (!Expand()) {
					std::cerr << "PoolAllocator::Request(): failed to allocate new  block" << std::endl;
					return nullptr;
				}
			}
			continue;
		}

		// Double check if the space is free or not
		/*if (block.nodes[index].free == false) {
			index = block.nodes[index].next;
		}*/

		// Set requested block as taken and update head node
		block.nodes[index].free = false;
		block.head = block.nodes[index].next;

		block.numUsed += 1;

		int memorySpace = index * _size;
		void* ptr = static_cast<char*>(block.address) + memorySpace;

		if (TRACK_MEMORY) {
			MemoryTracker::Instance().StartTracking(Allocator::Pool, _id, ptr, _size, tag);
		}

		return ptr;
	}

	return nullptr;
}

bool PoolAllocator::Free(void *ptr)
{
	if (ptr == nullptr) {
		std::cerr << "PoolAllocator::Free(): input pointer is nullptr" << std::endl;
		return false;
	}

	for (Block& block : _blocks) {

		// Casting to char pointer to allow for byte-wise arithmetics
		char* startAddress = static_cast<char*>(block.address);
		char* elementAddress = static_cast<char*>(ptr);
		ptrdiff_t byteDiff = elementAddress - startAddress;

		// Block bounds check (is element in this block?)
		if (byteDiff < 0 || byteDiff >= _n * _size) {
			continue;
		}

		// Alignment safety check
		if (byteDiff % _size != 0) {
			std::cerr << "PoolAllocator::Free(): input pointer is misaligned with the pool" << std::endl;
			return false;
		}

		int index = byteDiff / _size;

		// Double free safety check
		if (block.nodes[index].free) {
			std::cerr << "PoolAllocator::Free(): memory is already free" << std::endl;
			return false;
		}

		block.nodes[index].free = true;
		block.nodes[index].next = block.head;
		block.head = index;

		block.numUsed -= 1;

		if (TRACK_MEMORY) {
			MemoryTracker::Instance().StopTracking(ptr);
		}

		return true;
	}

	// Input pointer was not within the range of any block
	std::cerr << "PoolAllocator::Free(): Input pointer does not belong to this pool" << std::endl;
	return false;
}

bool PoolAllocator::GetUsed(int index) {
	int i = index % _n;
	int k = index / _n;
	return _blocks[k].nodes[i].free;
}

int PoolAllocator::GetNumSlots()
{
	return _n * _blocks.size();
}

// Debug
void* PoolAllocator::GetAdress(size_t index) {
	return _blocks.at(index).address;
}
