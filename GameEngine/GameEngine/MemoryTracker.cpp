#include "MemoryTracker.h"
#include <iostream>

void MemoryTracker::StartTracking(Allocator allocator, int allocatorId, void* ptr, size_t size, std::string tag)
{
	Allocation allocation;
	allocation.allocator = allocator;
	allocation.allocatorId = allocatorId;
	allocation.ptr = ptr;
	allocation.size = size;
	allocation.tag = tag;
	allocation.timestamp = std::chrono::system_clock::now();

	_allocations.emplace(ptr, allocation);
}

void MemoryTracker::StopTracking(void* ptr)
{
	_allocations.erase(ptr);
}

bool MemoryTracker::GetAllocation(void* ptr, Allocation& allocation)
{
	auto element = _allocations.find(ptr);
	if (element == _allocations.end()) {
		std::cerr << "MemoryTracker::GetAllocation(): allocation at pointer is not being tracked" << std::endl;
		return false;
	}

	allocation = element->second; // returns the item (allocation)
	return true;
}

std::unordered_map<void*, Allocation> MemoryTracker::GetAllocations()
{
	return _allocations;
}

bool MemoryTracker::GetAllocatorStats(int id, StackStats& stats)
{
	auto element = _stackAllocators.find(id);
	if (element == _stackAllocators.end()) {
		std::cerr << "MemoryTracker::GetAllocatorStats(): allocator with input id is not being tracked" << std::endl;
		return false;
	}
	else {
		stats = element->second;
		return true;
	}
}

std::unordered_map<int, StackStats> MemoryTracker::GetStackAllocators()
{
	return _stackAllocators;
}

bool MemoryTracker::GetAllocatorStats(int id, PoolStats& stats)
{
	auto element = _poolAllocators.find(id);
	if (element == _poolAllocators.end()) {
		std::cerr << "MemoryTracker::GetAllocatorStats(): allocator with input id is not being tracked" << std::endl;
		return false;
	}
	else {
		stats = element->second;
		return true;
	}
}

std::unordered_map<int, PoolStats> MemoryTracker::GetPoolAllocators()
{
	return _poolAllocators;
}

bool MemoryTracker::GetAllocatorStats(int id, BuddyStats& stats)
{
	auto element = _buddyAllocators.find(id);
	if (element == _buddyAllocators.end()) {
		std::cerr << "MemoryTracker::GetAllocatorStats(): allocator with input id is not being tracked" << std::endl;
		return false;
	}
	else {
		stats = element->second;
		return true;
	}
}

std::unordered_map<int, BuddyStats> MemoryTracker::GetBuddyAllocators()
{
	return _buddyAllocators;
}

void MemoryTracker::TrackAllocator(int id, const StackStats& stats)
{
	_stackAllocators[id] = stats;
}

void MemoryTracker::TrackAllocator(int id, const PoolStats& stats)
{
	_poolAllocators[id] = stats;
}

void MemoryTracker::TrackAllocator(int id, const BuddyStats& stats)
{
	_buddyAllocators[id] = stats;
}

void MemoryTracker::RemoveAllocator(int id, Allocator allocator)
{
	switch (allocator)
	{
	case Allocator::Stack:
		_stackAllocators.erase(id);
		break;
	case Allocator::Pool:
		_poolAllocators.erase(id);
		break;
	case Allocator::Buddy:
		_buddyAllocators.erase(id);
		break;
	default:
		break;
	}
}
