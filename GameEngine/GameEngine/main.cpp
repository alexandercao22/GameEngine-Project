#include "TestCases.h"
#include "PackageManager.h"
#include "SceneManager.h"
#include "ResourceManager.h"

#include "raylib.h"
#include <chrono>

#include <filesystem>

int main() {
	SceneManager sceneManager;
	sceneManager.Init();

	while (!WindowShouldClose()) {
		sceneManager.Update();
		sceneManager.RenderUpdate();
	}

	CloseWindow();

	return 0;
}