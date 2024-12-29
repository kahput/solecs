#include "ecs.h"

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

void system_move(Entity entity) {
	Rectangle* rect = (Rectangle*)ecs_fetch_component(entity, 0);
	Vector2* vel = (Vector2*)ecs_fetch_component(entity, 1);
	rect->x += vel->x;
	rect->y += vel->y;

	if (rect->x + rect->width >= 1280)
		vel->x *= -1;
	if (rect->x <= 0)
		vel->x *= -1;
}

void system_draw(Entity entity) {
	Rectangle* rect = (Rectangle*)ecs_fetch_component(entity, 0);
	Color* color = (Color*)ecs_fetch_component(entity, 2);

	DrawRectangleRec(*rect, *color);
}

int main(void) {
	const int SCREEN_WIDTH = 1280;
	const int SCREEN_HEIGHT = 720;
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello");
	ecs_startup(3, sizeof(Rectangle), sizeof(Vector2), sizeof(Color));
	ecs_attach_system(system_move, 2, 0, 1);
	ecs_attach_system(system_draw, 2, 0, 2);

	Entity player = ecs_create_entity();
	ecs_attach_component(player, 0, &(Rectangle){ .x = 100.f, .y = 100.f, .width = 50.f, .height = 50.f });
	ecs_attach_component(player, 1, &(Vector2){ 0.f, 0.f });
	ecs_attach_component(player, 2, &(Color){ .r = 25, .g = 100, .b = 25, .a = 255 });
	Entity obstacle = ecs_create_entity();
	ecs_attach_component(obstacle, 0, &(Rectangle){ .x = 200.f, .y = 300.f, .width = 400.f, .height = 100.f });
	ecs_attach_component(obstacle, 2, &(Color){ .r = 50, .g = 50, .b = 50, .a = 255 });
	Entity enemy = ecs_create_entity();
	ecs_attach_component(enemy, 0, &(Rectangle){ .x = 960.f, .y = 600.f, .width = 50.f, .height = 50.f });
	ecs_attach_component(enemy, 1, &(Vector2){ 0.001f, 0.f });
	ecs_attach_component(enemy, 2, &(Color){ .r = 100, .g = 25, .b = 25, .a = 255 });

	Rectangle* player_rec = (Rectangle*)ecs_fetch_component(player, 0);

	printf("Player rect: { x = %.2f, y = %.2f, width = %.2f, height = %.2f }\n", player_rec->x, player_rec->y, player_rec->width, player_rec->height);
	Rectangle* enemy_rec = (Rectangle*)ecs_fetch_component(enemy, 0);
	printf("Enemy rect before: { x = %.2f, y = %.2f, width = %.2f, height = %.2f }\n", enemy_rec->x, enemy_rec->y, enemy_rec->width, enemy_rec->height);

	SetExitKey(KEY_ESCAPE);
	while (!WindowShouldClose()) {
		ecs_update(GetFrameTime());

		Vector2* vel = (Vector2*)ecs_fetch_component(player, 1);
		if (vel) {
		vel->x = IsKeyDown(KEY_D) - IsKeyDown(KEY_A);
		vel->y = IsKeyDown(KEY_S) - IsKeyDown(KEY_W);

		}

		BeginDrawing();
		ClearBackground(BLACK);

		if (IsKeyPressed(KEY_H))
			ecs_destroy_entity(enemy);
		if (IsKeyPressed(KEY_L)) {
			enemy = ecs_create_entity();
			ecs_attach_component(enemy, 0, &(Rectangle){ .x = 960.f, .y = 600.f, .width = 50.f, .height = 50.f });
			ecs_attach_component(enemy, 1, &(Vector2){ 1.f, 0.f });
			ecs_attach_component(enemy, 2, &(Color){ .r = 100, .g = 25, .b = 25, .a = 255 });
		}
		if (IsKeyPressed(KEY_Q))
			ecs_destroy_entity(player);
		if (IsKeyPressed(KEY_E)) {
			player = ecs_create_entity();
			ecs_attach_component(player, 0, &(Rectangle){ .x = 100.f, .y = 100.f, .width = 50.f, .height = 50.f });
			ecs_attach_component(player, 1, &(Vector2){ 0.f, 0.f });
			ecs_attach_component(player, 2, &(Color){ .r = 25, .g = 100, .b = 25, .a = 255 });
		}
		EndDrawing();
	}

	// printf("Enemy rect after: { x = %.2f, y = %.2f, width = %.2f, height = %.2f }\n", enemy_rec->x, enemy_rec->y, enemy_rec->width, enemy_rec->height);

	ecs_shutdown();
	CloseWindow();
	return 0;
}
