#include "raylib.h"

#include "SceneManager.h"

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