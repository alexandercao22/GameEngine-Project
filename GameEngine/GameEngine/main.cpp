#include "TestCases.h"
#include "Interface.h"
#include "PackageManager.h"
#include "Scene.h"
#include "ResourceManager.h"

#include "raylib.h"
#include <chrono>

#include <filesystem>

int main() {
	//Interface interface;

	Scene scene;
	scene.Init();

	while (!WindowShouldClose()) {
		//interface.Update();

		scene.Update();
		scene.RenderUpdate();
	}

	CloseWindow();

	return 0;
}