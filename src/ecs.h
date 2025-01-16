#pragma once

#include "defines.h"

#include <assert.h>

typedef u32 Entity;
typedef struct Query {
	u32* entities;
	u32 count, id;
	void* (*fetch_components)(struct Query* query, u32 component_id);
} Query;
typedef void (*SystemFunc)(Query* query);

void ecs_startup(u32 component_count, ...);
void ecs_shutdown();

Entity ecs_create_entity();
void ecs_destroy_entity(Entity entity);
bool ecs_validate_entity(Entity entity);

void ecs_attach_component(Entity entity, u32 component_id, void* data);
void ecs_detach_component(Entity entity, u32 component_id);
void* ecs_fetch_component(Entity entity, u32 component_id);
bool ecs_has_component(Entity entity, u32 component_id);

void ecs_attach_system(SystemFunc system_ptr, u32 component_count, ...);

void ecs_update(f64 dt);
