#pragma once

#include "defines.h"

#include <assert.h>

typedef u32 Entity;
typedef struct {
	u32 count, id;
	void* (*primary)(u32 primary, u32 index);
	void* (*secondary)(u32 primary, u32 index, u32 component_id);
} Query;
typedef void (*SystemPointer)(Query* query);

void ecs_startup(u32 component_count, ...);
void ecs_shutdown();

Entity ecs_create_entity();
void ecs_destroy_entity(Entity entity);
bool ecs_validate_entity(Entity entity);

void ecs_attach_component(Entity entity, u32 component_id, void* data);
void ecs_detach_component(Entity entity, u32 component_id);
void* ecs_fetch_component(Entity entity, u32 component_id);
bool ecs_has_component(Entity entity, u32 component_id);

void ecs_attach_system(SystemPointer system_ptr, u32 component_count, ...);

void ecs_update(f64 dt);
