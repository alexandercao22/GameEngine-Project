#include "BuddyAllocator.h"

#include <iostream>
#include <string>
#include <cmath>
#include <malloc.h>

#include "raylib.h"
#include "imgui.h"
#include "rlImGui.h"

int BuddyAllocator::_nextId = 0; // Set the initial id

BuddyAllocator::~BuddyAllocator()
{
	free(_memory);
	free(_buddies);

	if (TRACK_MEMORY) {
		MemoryTracker::Instance().RemoveAllocator(_id, Allocator::Buddy);
	}
}

bool BuddyAllocator::Init(unsigned int size)
{
	// Check if size is smaller than lower limit (32)
	if (size < _maxDepthSize) {
		std::cerr << "BuddyAllocator::Init(): Minimun allocator size is: " << std::to_string(_maxDepthSize) << std::endl;
		return false;
	}

	// Check if size is of base 2
	float log2Size = std::log(size) / std::log(2);
	if (ceilf(log2Size) != log2Size) {
		std::cerr << "BuddyAllocator::Init(): Size is required to be of base 2" << std::endl;
		return false;
	}

	_memory = malloc(size);
	if (!_memory) {
		std::cerr << "BuddyAllocator::Init(): Failed to allocate memory" << std::endl;
		return false;
	}

	// Calculate how many slots are needed
	for (int i = 0; i < log2Size - (std::log(_maxDepthSize) / std::log(2)) + 1; i++) {
		_numBuddies += std::pow(2, i);
	}
	_buddies = (Buddy *)malloc(static_cast<size_t>(sizeof(Buddy) * _numBuddies));
	if (!_buddies) {
		std::cerr << "BuddyAllocator::Init(): Failed to allocate buddies" << std::endl;
		return false;
	}

	int level = 0;
	int buddiesToInit = std::pow(2, level); // Amount of buddies to initialize on current level
	int buddiesInit = 0; // Amount of buddies initialized on current level
	for (int i = 0; i < _numBuddies; i++) {
		_buddies[i].size = size / std::pow(2, level);
		_buddies[i].state = 0;
		_buddies[i].ptr = (char *)_memory + (_buddies[i].size * buddiesInit);

		buddiesInit++;
		if (buddiesInit == buddiesToInit) {
			level++;
			buddiesToInit = std::pow(2, level);
			buddiesInit = 0;
		}
	}

	_size = size;

	_id = _nextId;
	_nextId++;

	if (TRACK_MEMORY) {
		MemoryTracker::Instance().TrackAllocator(_id, GetStats());
	}

	return true;
}

void *BuddyAllocator::Request(unsigned int size, std::string tag)
{
#ifdef DEBUG
	std::cout << "Request(" << size << ")" << std::endl;
#endif
	if (size > _buddies[0].size) {
		std::cerr << "BuddyAllocator::Request(): The requested amount is too large" << std::endl;
		return nullptr;
	}

	if (size > _size - _usedMemory) {
		std::cerr << "BuddyAllocator::Request(): There's no free memory left" << std::endl;
		return nullptr;
	}

	int i = 0;
	int maxIndex = (1 << (int)(std::log2(_size) - std::log2(size) + 1)) - 1;
	while (i < maxIndex) {
		Buddy *current = &_buddies[i];
		if (current->state == 0) {
			if (size <= current->size) {
				if (size > current->size / 2 || current->size == _maxDepthSize) { // Found a free slot for the desired size
					current->state = 1;
					_usedMemory += current->size;
					if (TRACK_MEMORY) {
						MemoryTracker::Instance().StartTracking(Allocator::Buddy, _id, current->ptr, current->size, tag);
					}
					return current->ptr;
				}
				else { // Split
					current->state = 2;
					i = i * 2 + 1;
				}
			}
		}
		else if (current->state == 1) {
			// If right side buddy go back until current is a left side buddy
			while (i % 2 == 0 && i < maxIndex - 1) {
				i = (i - 1) / 2;
			}
			i++;
		}
		else if (current->state == 2) {
			if (size > current->size / 2) {
				while (i % 2 == 0 && i < maxIndex - 1) {
					i = (i - 1) / 2;
				}
				i++;
				continue;
			}
			i = i * 2 + 1;
		}
	}

	std::cerr << "BuddyAllocator::Request(): Could not find a free slot for the requested size" << std::endl;

	return nullptr;
}

bool BuddyAllocator::Free(void *element)
{
	char *elementPtr = (char *)element;
	if (elementPtr < (char *)_memory || elementPtr > (char *)_buddies[_numBuddies - 1].ptr) {
		std::cerr << "BuddyAllocator::Free(): Element is not within this allocator" << std::endl;
		return false;
	}

	int i = 0;
	Buddy *current = nullptr;
	Buddy *buddy = nullptr;
	for (i = 0; i < _numBuddies; i++) {
		if (_buddies[i].ptr == element && _buddies[i].state == 1) {
			current = &_buddies[i];
			current->state = 0;
			_usedMemory -= current->size;
			if (TRACK_MEMORY) {
				MemoryTracker::Instance().StopTracking(current->ptr);
			}

			if (i % 2 == 1) { // Current is left
				// Buddy is right
				buddy = &_buddies[i + 1];
			}
			else { // Current is right
				// Buddy is left
				buddy = &_buddies[i - 1];
			}
			break;
		}
	}

	if (buddy == nullptr) {
		std::cerr << "BuddyAllocator::Free(): Could not find the buddy of the freed element" << std::endl;
		return false;
	}

	while (i > 0) {
		if (buddy->state == 0) {
			i = (i - 1) / 2;
			Buddy *parent = &_buddies[i];
			parent->state = 0;
			current = parent;
		}
		else {
			break;
		}

		if (i % 2 == 1) { // Current is left
			// Buddy is right
			buddy = &_buddies[i + 1];
		}
		else { // Current is right
			// Buddy is left
			buddy = &_buddies[i - 1];
		}
	}

	return true;
}

BuddyStats BuddyAllocator::GetStats()
{
	BuddyStats stats;
	stats.capacity = _size;
	stats.usedMemory = _usedMemory;

	return stats;
}

void *BuddyAllocator::GetAddress()
{
	if (!_memory) {
		std::cerr << "BuddyAllocator::GetAddress(): Buddy allocator is uninitialized" << std::endl;
		return nullptr;
	}

	return _memory;
}

void BuddyAllocator::PrintStates()
{
	for (int i = 0; i < _numBuddies; i++) {
		std::cout << i << " ";
	}
	std::cout << std::endl;
	for (int i = 0; i < _numBuddies; i++) {
		int digits = 0;
		std::string space = "";
		if (i == 0) {
			digits = 1;
		}
		else {
			int temp = i;
			while (temp > 0) {
				temp /= 10;
				digits++;
				space += " ";
			}
		}

		std::cout << space << _buddies[i].state;
	}
	std::cout << std::endl << std::endl;
}

void BuddyAllocator::DrawInterface()
{
	ImDrawList *draw = ImGui::GetWindowDrawList();
	ImVec2 origin = ImGui::GetCursorScreenPos();

	const float blockHeight = 20.0f;
	const float blockSpacing = 8.0f;
	const float totalWidth = 600.0f;

	int maxLevel = std::log2(_size) - std::log2(_maxDepthSize);

	for (int level = 0; level <= maxLevel; level++) {
		int numBlocks = 1 << level;
		float y = origin.y + level * (blockHeight + blockSpacing);
		int index = numBlocks - 1;
		for (int i = 0; i < numBlocks; i++) {
			float x0 = floorf(origin.x + (float(i) / numBlocks) * totalWidth);
			float x1 = floorf(origin.x + (float(i + 1) / numBlocks) * totalWidth);

			ImU32 color = IM_COL32(200, 200, 200, 255);
			if (_buddies[index + i].state == 0) {
				color = IM_COL32(0, 255, 0, 255);
			}
			else if (_buddies[index + i].state == 1) {
				color = IM_COL32(255, 0, 0, 255);
			}
			else if (_buddies[index + i].state == 2) {
				color = IM_COL32(255, 255, 0, 255);
			}

			// Draw rectangle
			draw->AddRectFilled(
				ImVec2(x0, y),
				ImVec2(x1, y + blockHeight),
				color
			);

			draw->AddRect(
				ImVec2(x0, y),
				ImVec2(x1, y + blockHeight),
				IM_COL32(0, 0, 0, 255)
			);
		}  
	}

	// Move cursor so ImGui continues below the diagram
	ImGui::Dummy(ImVec2(totalWidth, (maxLevel + 1) *(blockHeight + blockSpacing)));
}
