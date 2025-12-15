#pragma once

#include <string>
#include <unordered_map>
#include <chrono>

struct StackStats {
	unsigned int capacity = 0;
	unsigned int usedMemory = 0;
};

struct PoolStats {
	unsigned int capacity = 0;
	unsigned int usedMemory = 0;
	int numBlocks = 0;
};

struct BuddyStats {
	unsigned int capacity = 0;
	unsigned int usedMemory = 0;
};

enum class Allocator {
	Stack,
	Pool,
	Buddy
};

struct Allocation {
	Allocator allocator;
	int allocatorId;
	void* ptr;
	size_t size = 0;	// Size in bytes
	std::string tag;	// Tag describing or categorizing the allocation
	std::chrono::time_point<std::chrono::system_clock> timestamp; // Creation timestamp
};

class MemoryTracker 
{
private:
	MemoryTracker() = default;
	~MemoryTracker() = default;

	// Stats of all tracked stack allocators (key = allocator id)
	std::unordered_map<int, StackStats> _stackAllocators;
	// Stats of all tracked pool allocators (key = allocator id)
	std::unordered_map<int, PoolStats> _poolAllocators;
	// Stats of all tracked buddy allocators (key = allocator id)
	std::unordered_map<int, BuddyStats> _buddyAllocators;

	// Keeps track of all tracked allocations using their pointers as keys for quick lookup
	std::unordered_map<void*, Allocation> _allocations;

public:
	// Singleton instance
	static MemoryTracker& Instance() {
		static MemoryTracker instance;
		return instance;
	}
	// Copy prevention
	MemoryTracker(const MemoryTracker&) = delete;
	MemoryTracker& operator=(const MemoryTracker&) = delete;

	// Starts tracking allocator if not already tracked, otherwise updates the allocator stats
	void TrackAllocator(int id, const StackStats& stats);
	// Starts tracking allocator if not already tracked, otherwise updates the allocator stats
	void TrackAllocator(int id, const PoolStats& stats);
	// Starts tracking allocator if not already tracked, otherwise updates the allocator stats
	void TrackAllocator(int id, const BuddyStats& stats);

	// Stops tracking the allocator with the given id
	void RemoveAllocator(int id, Allocator allocator);

	// Records a new allocation
	void StartTracking(Allocator allocator, int allocatorId, void* ptr, size_t size, std::string tag);
	// Removes an allocation from the record
	void StopTracking(void* ptr);

	// Gets information about the allocation at the given pointer
	bool GetAllocation(void* ptr, Allocation& allocation);
	// Gets all currently tracked allocations
	std::unordered_map<void*, Allocation> GetAllocations();

	// Gets the stats of a tracked allocator with the given id
	bool GetAllocatorStats(int id, StackStats& stats);
	// Gets the stats of all trackeded stack allocators
	std::unordered_map<int, StackStats> GetStackAllocators();

	// Gets the stats of a tracked allocator with the given id
	bool GetAllocatorStats(int id, PoolStats& stats);
	// Gets the stats of all tracked pool allocators
	std::unordered_map<int, PoolStats> GetPoolAllocators();

	// Gets the stats of a tracked allocator with the given id
	bool GetAllocatorStats(int id, BuddyStats& stats);
	// Gets the stats of all tracked buddy allocators
	std::unordered_map<int, BuddyStats> GetBuddyAllocators();


	// UI Functions
};

