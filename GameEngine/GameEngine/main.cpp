#include "TestCases.h"
#include "Interface.h"
#include "PackageManager.h"
#include "SceneManager.h"
#include "ResourceManager.h"

#include "raylib.h"
#include <chrono>

#include <filesystem>

int main() {
	//Interface interface;

	SceneManager sceneManager;
	sceneManager.Init();

	while (!WindowShouldClose()) {
		//interface.Update();

		sceneManager.Update();
		sceneManager.RenderUpdate();
	}

	CloseWindow();

	return 0;
}