#pragma once

#include "defines.h"

#include <assert.h>

typedef u32 Entity;
typedef struct {
	void** component_data;
	u32 count;
} ECSIterator;
typedef void (*SystemPointer)(ECSIterator*);

void ecs_startup(u32 component_count, ...);
void ecs_shutdown();

Entity create_entity();

void add_component(Entity e, u32 component_id, void* data);
void* get_component(Entity e, u32 component_id);

void ecs_system(SystemPointer system_ptr, u32 component_count, ...);

void* ecs_field(ECSIterator* itr, u32 component_id);
void ecs_update(f64 dt);
