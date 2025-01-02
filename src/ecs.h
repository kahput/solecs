#pragma once

#include "defines.h"

#include <assert.h>

#define MAX_COMPONENTS 32
#define MAX_SYSTEMS 8

typedef struct {
	void* data[MAX_COMPONENTS];
	u32 count;
} ComponentView;
typedef u32 Entity;
typedef void (*SystemPointer)(ComponentView*);

void ecs_startup(u32 component_count, ...);
void ecs_shutdown();

Entity ecs_entity_create();
void ecs_entity_destory(Entity entity);
bool ecs_entity_validate(Entity entity);

void ecs_component_attach(Entity entity, u32 component_id, void* data);
void ecs_component_detach(Entity entity, u32 component_id);
bool ecs_component_status(Entity entity, u32 component_id);
void* ecs_component_fetch(Entity entity, u32 component_id);
void* ecs_components_fetch(ComponentView* view, u32 component_id);

void ecs_system_attach(SystemPointer system_ptr, u32 component_count, ...);

void ecs_update(f64 dt);
