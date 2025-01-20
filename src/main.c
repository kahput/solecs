#include "ecs.h"

#include <raylib.h>
#include <raymath.h>

typedef enum {
	COMPONENT_RECT,
	COMPONENT_POSITION,
	COMPONENT_COLOR,

	COMPONENT_COUNT
} Components;

// void system_move(Query* query) {
// 	Rectangle* rects = query->fetch_components(query, COMPONENT_RECT);
// 	Vector2* velocities = query->fetch_components(query, COMPONENT_POSITION);
//
// 	for (int i = 0; i < query->count; i++) {
// 		rects[i].x += velocities[i].x;
// 		rects[i].y += velocities[i].y;
//
// 		if (rects[i].x + rects[i].width >= 1280)
// 			velocities[i].x *= -1;
// 		if (rects[i].x <= 0)
// 			velocities[i].x *= -1;
// 	}
// }

// void system_draw(Query* query) {
// 	Rectangle* rects = query->fetch_components(query, COMPONENT_RECT);
// 	Color* colors = query->fetch_components(query, COMPONENT_COLOR);
//
// 	for (int i = 0; i < query->count; i++) {
// 		Rectangle* rect = &rects[i];
// 		Color* color = &colors[i];
//
// 		DrawRectangleRec(*rect, *color);
// 	}
// }

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
