#include <raylib.h>
#include <raymath.h>

int main(void) {
	const int SCREEN_WIDTH = 1280;
	const int SCREEN_HEIGHT = 720;
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello");

	SetExitKey(KEY_ESCAPE);
	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BLACK);

		EndDrawing();
	}

	CloseWindow();
	return 0;
}
