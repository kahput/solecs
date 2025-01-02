#include "ecs.h"

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#define MIN(a, b) a < b ? a : b
#define MAX(a, b) a > b ? a : b

void system_move(ComponentView* view) {
	Rectangle* rect = (Rectangle*)ecs_components_fetch(view, 0);
	Vector2* vel = (Vector2*)ecs_components_fetch(view, 1);

	for (int i = 0; i < view->count; i++) {
		rect[i].x += vel[i].x;
		rect[i].y += vel[i].y;
		// printf("Rect: (%.2f, %.2f)\n", rect[i].x, rect[i].y);
		if (rect[i].x + rect[i].width >= 1280 || rect[i].x <= 0)
			vel[i].x *= -1;
		if (rect[i].y + rect[i].height >= 720 || rect[i].y <= 0)
			vel[i].y *= -1;
	}
}

void system_draw(ComponentView* view) {
	Rectangle* rect = (Rectangle*)ecs_components_fetch(view, 0);
	Color* color = (Color*)ecs_components_fetch(view, 2);

	for (int i = 0; i < view->count; i++) {
		DrawRectangleRec(rect[i], color[i]);
	}
}

int main(void) {
	const int SCREEN_WIDTH = 1280;
	const int SCREEN_HEIGHT = 720;
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello");
	ecs_startup(3, sizeof(Rectangle), sizeof(Vector2), sizeof(Color));
	ecs_system_attach(system_move, 2, 0, 1);
	ecs_system_attach(system_draw, 2, 0, 2);

	Entity player = ecs_entity_create();
	ecs_component_attach(player, 0, &(Rectangle){ .x = 100.f, .y = 100.f, .width = 50.f, .height = 50.f });
	ecs_component_attach(player, 1, &(Vector2){ 0.f, 0.f });
	ecs_component_attach(player, 2, &(Color){ .r = 25, .g = 100, .b = 25, .a = 255 });
	Entity obstacle = ecs_entity_create();
	ecs_component_attach(obstacle, 0, &(Rectangle){ .x = 200.f, .y = 300.f, .width = 400.f, .height = 100.f });
	ecs_component_attach(obstacle, 2, &(Color){ .r = 50, .g = 50, .b = 50, .a = 255 });
	Entity enemy = ecs_entity_create();
	ecs_component_attach(enemy, 0, &(Rectangle){ .x = 960.f, .y = 600.f, .width = 50.f, .height = 50.f });
	ecs_component_attach(enemy, 1, &(Vector2){ 0.001f, 0.f });
	ecs_component_attach(enemy, 2, &(Color){ .r = 100, .g = 25, .b = 25, .a = 255 });

	Rectangle* player_rec = (Rectangle*)ecs_component_fetch(player, 0);

	printf("Player rect: { x = %.2f, y = %.2f, width = %.2f, height = %.2f }\n", player_rec->x, player_rec->y, player_rec->width, player_rec->height);
	Rectangle* enemy_rec = (Rectangle*)ecs_component_fetch(enemy, 0);
	printf("Enemy rect before: { x = %.2f, y = %.2f, width = %.2f, height = %.2f }\n", enemy_rec->x, enemy_rec->y, enemy_rec->width, enemy_rec->height);

	SetExitKey(KEY_ESCAPE);
	while (!WindowShouldClose()) {
		ecs_update(GetFrameTime());

		if (ecs_entity_validate(player)) {
			Vector2* vel = (Vector2*)ecs_component_fetch(player, 1);
			vel->x = IsKeyDown(KEY_D) - IsKeyDown(KEY_A);
			vel->y = IsKeyDown(KEY_S) - IsKeyDown(KEY_W);
		}

		BeginDrawing();
		ClearBackground(BLACK);

		if (IsKeyPressed(KEY_H))
			ecs_entity_destory(enemy);
		if (IsKeyPressed(KEY_L)) {
			enemy = ecs_entity_create();
			ecs_component_attach(enemy, 0, &(Rectangle){ .x = 960.f, .y = 600.f, .width = 50.f, .height = 50.f });
			ecs_component_attach(enemy, 1, &(Vector2){ 1.f, 0.f });
			ecs_component_attach(enemy, 2, &(Color){ .r = 100, .g = 25, .b = 25, .a = 255 });
		}
		if (IsKeyPressed(KEY_Q))
			ecs_entity_destory(player);
		if (IsKeyPressed(KEY_E)) {
			player = ecs_entity_create();
			ecs_component_attach(player, 0, &(Rectangle){ .x = 100.f, .y = 100.f, .width = 50.f, .height = 50.f });
			ecs_component_attach(player, 1, &(Vector2){ 0.f, 0.f });
			ecs_component_attach(player, 2, &(Color){ .r = 25, .g = 100, .b = 25, .a = 255 });
		}
		EndDrawing();
	}

	// printf("Enemy rect after: { x = %.2f, y = %.2f, width = %.2f, height = %.2f }\n", enemy_rec->x, enemy_rec->y, enemy_rec->width, enemy_rec->height);

	ecs_shutdown();
	CloseWindow();
	return 0;
}
