#pragma once

#include "defines.h"

#include <assert.h>

typedef u32 Entity;
typedef void (*SystemPointer)(Entity entity);

void ecs_startup(u32 component_count, ...);
void ecs_shutdown();

Entity ecs_create_entity();
void ecs_destroy_entity(Entity entity);

void ecs_attach_component(Entity entity, u32 component_id, void* data);
void ecs_detach_component(Entity entity, u32 component_id);
void* ecs_fetch_component(Entity entity, u32 component_id);

void ecs_attach_system(SystemPointer system_ptr, u32 component_count, ...);

void ecs_update(f64 dt);
