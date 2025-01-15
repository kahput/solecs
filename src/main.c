#include "ecs.h"

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

typedef enum {
	COMPONENT_RECT,
	COMPONENT_POSITION,
	COMPONENT_COLOR,

	COMPONENT_COUNT
} Components;

void system_move(Query* query) {
	for (int i = 0; i < query->count; i++) {
		Vector2* vel = (Vector2*)query->primary(query->id, i);
		Rectangle* rect = (Rectangle*)query->secondary(query->id, i, COMPONENT_RECT);
		rect->x += vel->x;
		rect->y += vel->y;

		if (rect->x + rect->width >= 1280)
			vel->x *= -1;
		if (rect->x <= 0)
			vel->x *= -1;
	}
}

void system_draw(Query* query) {
	for (int i = 0; i < query->count; i++) {
		Rectangle* rect = (Rectangle*)query->primary(query->id, i);
		Color* color = (Color*)query->secondary(query->id, i, COMPONENT_COLOR);

		DrawRectangleRec(*rect, *color);
	}
}

int main(void) {
	const int SCREEN_WIDTH = 1280;
	const int SCREEN_HEIGHT = 720;
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello");
	ecs_startup(COMPONENT_COUNT, sizeof(Rectangle), sizeof(Vector2), sizeof(Color));
	ecs_attach_system(system_move, 2, COMPONENT_POSITION, COMPONENT_RECT);
	ecs_attach_system(system_draw, 2, COMPONENT_RECT, COMPONENT_COLOR);

	Entity player = ecs_create_entity();
	ecs_attach_component(player, COMPONENT_RECT, &(Rectangle){ .x = 100.f, .y = 100.f, .width = 50.f, .height = 50.f });
	ecs_attach_component(player, COMPONENT_POSITION, &(Vector2){ 0.f, 0.f });
	ecs_attach_component(player, COMPONENT_COLOR, &(Color){ .r = 25, .g = 100, .b = 25, .a = 255 });
	Entity obstacle = ecs_create_entity();
	ecs_attach_component(obstacle, COMPONENT_RECT, &(Rectangle){ .x = 200.f, .y = 300.f, .width = 400.f, .height = 100.f });
	ecs_attach_component(obstacle, COMPONENT_COLOR, &(Color){ .r = 50, .g = 50, .b = 50, .a = 255 });
	Entity enemy = ecs_create_entity();
	ecs_attach_component(enemy, COMPONENT_RECT, &(Rectangle){ .x = 960.f, .y = 600.f, .width = 50.f, .height = 50.f });
	ecs_attach_component(enemy, COMPONENT_POSITION, &(Vector2){ 0.001f, 0.f });
	ecs_attach_component(enemy, COMPONENT_COLOR, &(Color){ .r = 100, .g = 25, .b = 25, .a = 255 });

	Rectangle* player_rec = (Rectangle*)ecs_fetch_component(player, COMPONENT_RECT);
	Rectangle* enemy_rec = (Rectangle*)ecs_fetch_component(enemy, COMPONENT_RECT);

	printf("Player_rec(%.2f, %.2f)\n", player_rec->x, player_rec->y);

	SetExitKey(KEY_ESCAPE);
	while (!WindowShouldClose()) {
		ecs_update(GetFrameTime());

		if (ecs_validate_entity(player)) {
			Vector2* vel = (Vector2*)ecs_fetch_component(player, 1);
			if (vel) {
				vel->x = IsKeyDown(KEY_D) - IsKeyDown(KEY_A);
				vel->y = IsKeyDown(KEY_S) - IsKeyDown(KEY_W);
			}
		}

		BeginDrawing();
		ClearBackground(BLACK);

		if (IsKeyPressed(KEY_H))
			ecs_destroy_entity(enemy);
		if (IsKeyPressed(KEY_L)) {
			enemy = ecs_create_entity();
			ecs_attach_component(enemy, COMPONENT_RECT, &(Rectangle){ .x = 960.f, .y = 600.f, .width = 50.f, .height = 50.f });
			ecs_attach_component(enemy, COMPONENT_POSITION, &(Vector2){ 1.f, 0.f });
			ecs_attach_component(enemy, COMPONENT_COLOR, &(Color){ .r = 100, .g = 25, .b = 25, .a = 255 });
		}
		if (IsKeyPressed(KEY_Q))
			ecs_destroy_entity(player);
		if (IsKeyPressed(KEY_E)) {
			player = ecs_create_entity();
			ecs_attach_component(player, COMPONENT_RECT, &(Rectangle){ .x = 100.f, .y = 100.f, .width = 50.f, .height = 50.f });
			ecs_attach_component(player, COMPONENT_POSITION, &(Vector2){ 0.f, 0.f });
			ecs_attach_component(player, COMPONENT_COLOR, &(Color){ .r = 25, .g = 100, .b = 25, .a = 255 });
		}
		EndDrawing();
	}

	ecs_shutdown();
	CloseWindow();
	return 0;
}
