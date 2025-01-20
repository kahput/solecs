#pragma once

#include "defines.h"

#include <assert.h>

typedef u32 Entity;
typedef u32 ComponentID;
typedef struct _world World;

World* ecs_startup();
void ecs_shutdown(World* world);

Entity ecs_create_entity(World* world);
void ecs_destroy_entity(World* world, Entity entity);
bool ecs_validate_entity(World* world, Entity entity);

void ecs_register_component(World* world, Entity entity);
void ecs_attach_component(World* world, Entity entity, u32 component_id, void* data);
void ecs_detach_component(World* world, Entity entity, u32 component_id);
void* ecs_fetch_component(World* world, Entity entity, u32 component_id);
bool ecs_has_component(World* world, Entity entity, u32 component_id);

void ecs_query(u32 component_count, ...);

void ecs_update(f64 dt);
