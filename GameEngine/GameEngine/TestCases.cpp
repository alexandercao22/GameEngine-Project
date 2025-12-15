#include "TestCases.h"

#include "PoolAllocator.h"
#include "StackAllocator.h"
#include "BuddyAllocator.h"
#include "Objects.h"

#include <chrono>
#include <iostream>
#include <random>


void PoolVSOS() {

	int loops = 10;

	int startObjects = 2000;
	std::cout << " ---- Testing PoolAllocation ---- " << std::endl;
	std::mt19937 rng(12345);

	const int FRAMES = 100'000;
	double poolTime[10];
	for (int k = 0; k < loops; k++) {

		int nrOfObjects = 0;
		auto t0 = std::chrono::high_resolution_clock::now();


		PoolAllocator pool;
		pool.Init(startObjects, sizeof(Enemy));
		std::vector<Enemy*> live;
		for (int f = 0; f < FRAMES; f++) {

			// Allocate some random objects
			int allocCount = rng() % 1000;
			for (int i = 0; i < allocCount; i++) {
				Enemy* e = (Enemy*)pool.Request();
				live.push_back(e);
				nrOfObjects++;
			}

			// Free some random objects
			int freeCount = rng() % 1000;
			for (int i = 0; i < freeCount && !live.empty(); i++) {
				int index = rng() % live.size();
				pool.Free(live[index]);
				live.erase(live.begin() + index);
			}

		}
		
		auto t1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> duration = t1 - t0;

		poolTime[k] = duration.count();
		std::cout << "live size: " << live.size() << std::endl;
		std::cout << "NrOfObjects created: " << nrOfObjects << std::endl;
		std::cout << "Execution time: " << poolTime[k] << " iteration " << k << std::endl;
		live.clear();
	}
	double result = 0;
	for (int i = 0; i < loops; i++) {
		result += poolTime[i];
	}
	result /= 10;
	std::cout << " ---- OS PoolAllocation Execution time: " << result << " ---- " << std::endl;
	std::cout << std::endl;

	std::cout << " ---- Testing OS new/delete ---- " << std::endl;

	std::vector<Enemy*> live2;
	for (int k = 0; k < loops; k++) {

		int nrOfObjects = 0;
		auto t0 = std::chrono::high_resolution_clock::now();
		for (int f = 0; f < FRAMES; f++) {

			// Allocate some random objects
			int allocCount = rng() % 1000;
			for (int i = 0; i < allocCount; i++) {
				Enemy* e = new Enemy;
				live2.push_back(e);
				nrOfObjects++;
			}

			// Free some random objects
			int freeCount = rng() % 1000;
			for (int i = 0; i < freeCount && !live2.empty(); i++) {
				int index = rng() % live2.size();
				delete live2[index];
				live2.erase(live2.begin() + index);
			}

		}
		auto t1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> duration = t1 - t0;

		std::cout << "live2 size: " << live2.size() << std::endl;
		std::cout << "NrOfObjects created: " << nrOfObjects << std::endl;
		// Remove remaining objects from the vector to prevent memory leaks
		for (int i = 0; i < live2.size(); i++) {
			delete live2[i];
		}
		poolTime[k] = duration.count();
		std::cout << "Execution time: " << poolTime[k] << " iteration " << k << std::endl;
		live2.clear();
	}
	result = 0;
	for (int i = 0; i < loops; i++) {
		result += poolTime[i];
	}
	result /= 10;
	std::cout << " ---- OS new/delete Execution average time: " << result << " ---- " << std::endl;

}

void StackVsOS() {
	std::cout << "Testing OS Allocator" << std::endl;
	// testing variables
	int nrOfIterations = 100'000;
	int nrOfObjects;

	std::mt19937 rng(12345);

	// Testing allocations framewise with "fixed size" enemies
	auto t0 = std::chrono::high_resolution_clock::now();
	std::vector<Enemy*> enemies;

	for (int i = 0; i < nrOfIterations; i++) {
		
		nrOfObjects = rng() % 1000;

		for (int k = 0; k < nrOfObjects; k++) {
			Enemy* enemy = new Enemy;
			enemies.push_back(enemy);
		}

		for (auto enemy : enemies) {
			delete enemy;
		}
		enemies.clear();
	}
	auto t1 = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> duration = t1 - t0;
	std::cout << "OS Allocator exection time" << duration.count() << std::endl;


	std::cout << "Testing StackAllocator: " << std::endl;
	// Testing allocation framewise with our Stack
	// reseting each iteration instead of deleting them
	StackAllocator firstStack;
	firstStack.Init(100000);
	t0 = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < nrOfIterations; i++) {
		nrOfObjects = rng() % 1000;
		for (int k = 0; k < nrOfObjects; k++) {
			void* ptr = firstStack.Request(sizeof(Enemy));
		}
		firstStack.Reset();

	}
	t1 = std::chrono::high_resolution_clock::now();
	duration = t1 - t0;

	std::cout << "StackAllocator exection time: " << duration.count() << std::endl;


	std::cout << "Testing OS Stack " << std::endl;
	// Testing variables in the OS Stack
	t0 = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < nrOfIterations; i++) {
		for (int k = 0; k < nrOfObjects; k++) {
			Enemy enemy;
			enemy.health = 100;
			enemy.legs = 2;
			enemy.tag = 'a';
		}
	}
	t1 = std::chrono::high_resolution_clock::now();

	duration = t1 - t0;

	std::cout << "OS Stack exection time: " << duration.count() << std::endl;

}

void TestAll() {
	int frames = 100'000;
	int objects = 5'000;
	double poolTime[10];
	double stackTime[10];
	double osTime[10];
	double buddyTime[10];
	
	std::cout << "Testing all allocators 100 000 iterations allocating and deallocating 10 enemies" << std::endl;
	for (int k = 0; k < 1; k++) {

		//std::cout << std::endl;
		//std::cout << "--- PoolAllocator ---" << std::endl;
		//auto t0 = std::chrono::high_resolution_clock::now();
		//std::vector<Enemy*> poolPtrs;
		//PoolAllocator pool;
		//pool.Init(objects, sizeof(Enemy));
		////for (int i = 0; i < frames; i++) {
		////	for (int y = 0; y < objects; y++) {
		////		Enemy* enemy = (Enemy*)pool.Request();
		////		poolPtrs.push_back(enemy);
		////	}
		////	for (Enemy* enemy : poolPtrs) {
		////		pool.Free(enemy);
		////	}
		////	poolPtrs.clear();
		////}
		//auto t1 = std::chrono::high_resolution_clock::now();
		//std::chrono::duration<double> duration = t1 - t0;
		//poolTime[k] = duration.count();
		//std::cout << "Execution time: " << duration.count() << std::endl;
		//std::cout << std::endl;
		//std::cout << "--- StackAllocator ---" << std::endl;

		//t0 = std::chrono::high_resolution_clock::now();
		//std::vector<Enemy*> stackPtrs;
		//StackAllocator stack;
		//stack.Init(objects * sizeof(Enemy));

		//for (int i = 0; i < frames; i++) {
		//	for (int y = 0; y < objects; y++) {
		//		Enemy* enemy = (Enemy*)stack.Request(sizeof(Enemy));
		//		poolPtrs.push_back(enemy);
		//	}
		//	if (frames % 1000 == 0)
		//		std::cout << "hej";
		//	stack.Reset();
		//	stackPtrs.clear();
		//}
		//t1 = std::chrono::high_resolution_clock::now();
	
		//duration = t1 - t0;
		//stackTime[k] = duration.count();
		//std::cout << "Execution time: " << duration.count() << std::endl;
		//std::cout << std::endl;
		//std::cout << "--- OSAllocator ---" << std::endl;
		//t0 = std::chrono::high_resolution_clock::now();

		//std::vector<Enemy*> OSPtrs;

		//for (int i = 0; i < frames; i++) {
		//	for (int y = 0; y < objects; y++) {
		//		Enemy* enemy = new Enemy;
		//		OSPtrs.push_back(enemy);
		//	}
		//	for (Enemy* enemy : OSPtrs) {
		//		delete enemy;
		//	}


		//	OSPtrs.clear();
		//}

		//t1 = std::chrono::high_resolution_clock::now();

		//duration = t1 - t0;
		//osTime[k] = duration.count();
		//std::cout << "Execution time: " << duration.count() << std::endl;
		//std::cout << std::endl;
		std::cout << "--- BuddyAllocator ---" << std::endl;
		auto t0 = std::chrono::high_resolution_clock::now();
		std::vector<Enemy*> buddyPtrs;
		BuddyAllocator buddy;
		buddy.Init(std::pow(2,18));

		for (int i = 0; i < frames; i++) {
			for (int y = 0; y < objects; y++) {
				Enemy* enemy = (Enemy*)buddy.Request(sizeof(Enemy));
				buddyPtrs.push_back(enemy);
			}
			for (Enemy* enemy : buddyPtrs) {
				buddy.Free(enemy);
			}

			buddyPtrs.clear();
		}
		auto t1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> duration = t1 - t0;
		buddyTime[k] = duration.count();
		std::cout << "Execution time: " << duration.count() << std::endl;
		std::cout << std::endl;

	}

	double result = 0;
	for (int i = 0; i < 10; i++) {
		result += poolTime[i];
	}
	result /= 10;
	std::cout << "average time PoolAllocator: " << result << std::endl;
	result = 0;
	for (int i = 0; i < 10; i++) {
		result += stackTime[i];
	}
	result /= 10;
	std::cout << "average time StackAllocator: " << result << std::endl;
	
	result = 0;
	for (int i = 0; i < 10; i++) {
		result += osTime[i];
	}
	result /= 10;
	std::cout << "average time OS Allocator: " << result << std::endl;
	
	result = 0;
	for (int i = 0; i < 10; i++) {
		result += buddyTime[i];
	}
	result /= 10;
	std::cout << "average time BuddyAllocator: " << result << std::endl;
}	