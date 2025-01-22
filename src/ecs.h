#pragma once

#include <stdbool.h>
#include <stdint.h>


#define MAX_COMPONENTS 32
#define MAX_SYSTEMS 8

typedef struct {
	void* data[MAX_COMPONENTS];
	uint32_t count;
} ComponentView;
typedef uint32_t Entity;
typedef void (*SystemPointer)(ComponentView*);

void ecs_startup(uint32_t component_count, ...);
void ecs_shutdown();

Entity ecs_entity_create();
void ecs_entity_destory(Entity entity);
bool ecs_entity_validate(Entity entity);

void ecs_component_attach(Entity entity, uint32_t component_id, void* data);
void ecs_component_detach(Entity entity, uint32_t component_id);
bool ecs_component_status(Entity entity, uint32_t component_id);
void* ecs_component_fetch(Entity entity, uint32_t component_id);
void* ecs_components_fetch(ComponentView* view, uint32_t component_id);

void ecs_system_attach(SystemPointer system_ptr, uint32_t component_count, ...);

void ecs_update(double dt);
