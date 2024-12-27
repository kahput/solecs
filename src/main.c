#include "ecs.h"

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

void system_move(ECSIterator* itr) {
	Rectangle* rect = (Rectangle*)ecs_field(itr, 0);
	Vector2* vel = (Vector2*)ecs_field(itr, 1);

	for (int i = 0; i < itr->count; i++) {
		rect[i].x += vel[i].x;
		rect[i].y += vel[i].y;
	}
}

void system_draw(ECSIterator* itr) {
	Rectangle* rect = (Rectangle*)ecs_field(itr, 0);
	Color* color = (Color*)ecs_field(itr, 2);

	for (int i = 0; i < itr->count; i++) {
		DrawRectangleRec(rect[i], color[i]);
	}
}

int main(void) {
	const int SCREEN_WIDTH = 1280;
	const int SCREEN_HEIGHT = 720;
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello");
	ecs_startup(3, sizeof(Rectangle), sizeof(Vector2), sizeof(Color));
	ecs_system(system_move, 2, 0, 1);
	ecs_system(system_draw, 2, 0, 2);
	printf("Does this work? %u\n", ~4);

	Entity player = create_entity();
	add_component(player, 0, &(Rectangle){ .x = 100.f, .y = 100.f, .width = 50.f, .height = 50.f });
	add_component(player, 1, &(Vector2){ 0.f, 0.f });
	add_component(player, 2, &(Color){ .r = 25, .g = 100, .b = 25, .a = 255 });
	Entity obstacle = create_entity();
	add_component(obstacle, 0, &(Rectangle){ .x = 200.f, .y = 300.f, .width = 400.f, .height = 100.f });
	add_component(obstacle, 2, &(Color){ .r = 50, .g = 50, .b = 50, .a = 255 });
	Entity enemy = create_entity();
	add_component(enemy, 0, &(Rectangle){ .x = 960.f, .y = 600.f, .width = 50.f, .height = 50.f });
	add_component(enemy, 1, &(Vector2){ 1.f, 0.f });
	add_component(enemy, 2, &(Color){ .r = 100, .g = 25, .b = 25, .a = 255 });

	Rectangle* player_rec = (Rectangle*)get_component(player, 0);

	printf("Player rect: { x = %.2f, y = %.2f, width = %.2f, height = %.2f }\n", player_rec->x, player_rec->y, player_rec->width, player_rec->height);
	Rectangle* enemy_rec = (Rectangle*)get_component(enemy, 0);
	printf("Enemy rect before: { x = %.2f, y = %.2f, width = %.2f, height = %.2f }\n", enemy_rec->x, enemy_rec->y, enemy_rec->width, enemy_rec->height);

	SetExitKey(KEY_ESCAPE);
	while (!WindowShouldClose()) {
		ecs_update(GetFrameTime());

		BeginDrawing();
		ClearBackground(BLACK);

		EndDrawing();
	}

	printf("Enemy rect after: { x = %.2f, y = %.2f, width = %.2f, height = %.2f }\n", enemy_rec->x, enemy_rec->y, enemy_rec->width, enemy_rec->height);

	ecs_shutdown();
	CloseWindow();
	return 0;
}
