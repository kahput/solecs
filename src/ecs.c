#include "ecs.h"
#include "defines.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMPONENTS 32
#define MAX_SYSTEMS 8

typedef struct {
	void* data;
	size_t element_size;
	u32 count, capacity;

	// Packed component data
	// u32* entity_to_index;
	// Entity* index_to_entity;
} CompnentArray;

typedef struct {
	SystemPointer ptr;
	u32 signature, count;
} System;

typedef struct {
	CompnentArray components[MAX_COMPONENTS];
	System systems[MAX_SYSTEMS];
	u32 num_components, num_systems;
	u32* entity_signatures;
	u32 entity_count, entity_capcity;
	ECSIterator itr;
} ECS;

static ECS g_world = { 0 };

void ecs_startup(u32 component_count, ...) {
	assert(component_count <= MAX_COMPONENTS);
	g_world.num_components = component_count;
	g_world.num_systems = g_world.entity_count = g_world.entity_capcity = 0;

	g_world.entity_signatures = NULL;

	g_world.itr = (ECSIterator){ 0 };
	g_world.itr.component_data = malloc(g_world.num_components * sizeof(void*));

	va_list args;
	va_start(args, component_count);
	for (int i = 0; i < component_count; i++) {
		g_world.components[i].element_size = va_arg(args, size_t);
		g_world.components[i].data = NULL;
		g_world.components[i].count = g_world.components[i].capacity = 0;
	}

	for (int i = 0; i < component_count; i++) {
		printf("INFO: COMPONENT: [%zuB] [ID %d] created.\n", g_world.components[i].element_size, i);
	}
	va_end(args);
}

void ecs_shutdown() {
	// Free component data
	for (int i = 0; i < g_world.num_components; i++)
		free(g_world.components[i].data);

	// Free entites
	free(g_world.entity_signatures);

	free(g_world.itr.component_data);
}

Entity create_entity() {
	static Entity next_id = 0;
	Entity new_id = next_id++;
	if (g_world.entity_count == g_world.entity_capcity) {
		g_world.entity_capcity = g_world.entity_capcity ? g_world.entity_capcity * 2 : 32;
		g_world.entity_signatures = realloc(g_world.entity_signatures, g_world.entity_capcity * sizeof(Entity));

		printf("INFO: ENTITY: entity_signatures resized to %d.\n", g_world.entity_capcity);
	}
	g_world.entity_count++;
	g_world.entity_signatures[new_id] = 0;
	printf("INFO: ENTITY: [ID %d] created. Entity count = %d\n", new_id, g_world.entity_count);
	return new_id;
}

void add_component(Entity e, u32 component_id, void* data) {
	assert(component_id < g_world.num_components);
	CompnentArray* array = &g_world.components[component_id];
	size_t element_size = g_world.components[component_id].element_size;

	g_world.entity_signatures[e] |= 1 << component_id;

	// TODO: Packed components
	if (array->count == array->capacity || e >= array->capacity) {
		array->capacity = array->capacity ? array->capacity * 2 < e ? e * 2 : array->capacity : 32;
		array->data = realloc(array->data, array->capacity * element_size);

		printf("INFO: COMPONENT: component_array[%d] resized to %d.\n", component_id, array->capacity);
	}

	memcpy((u8*)array->data + e * element_size, data, element_size);
	printf("INFO: ENTITY: Component[ID %d | %zuB] added to entity [ID %d | Sig %d].\n", component_id, element_size, e, g_world.entity_signatures[e]);
	array->count++;
}

void* get_component(Entity e, u32 component_id) {
	assert(component_id < g_world.num_components);
	CompnentArray* array = &g_world.components[component_id];
	size_t element_size = g_world.components[component_id].element_size;

	return ((u8*)array->data + e * element_size);
	return NULL;
}

void ecs_system(SystemPointer system_ptr, u32 component_count, ...) {
	assert(component_count <= MAX_COMPONENTS && g_world.num_systems < MAX_SYSTEMS);
	System* system = &g_world.systems[g_world.num_systems++];

	va_list args;
	va_start(args, component_count);
	for (int i = 0; i < component_count; i++) {
		system->signature |= 1 << va_arg(args, u32);
	}
	system->ptr = system_ptr;
	system->count = component_count;
	va_end(args);

	// TODO: Move out to update using updated_entities list

	printf("INFO: SYSTEM: System [Idx %d | Sig %d] added\n", (g_world.num_systems - 1), system->signature);
}

void* ecs_field(ECSIterator* itr, u32 component_id) {
	return itr->component_data[component_id];
}

void ecs_update(f64 dt) {
	for (int sys = 0; sys < g_world.num_systems; sys++) {
		System* system = &g_world.systems[sys];
		g_world.itr.count = 0;
		Entity relevant_entities[g_world.entity_count];

		// Check every entity signature against system signature.
		// Create a packed ComponentArray of all relevant entities
		for (int entity = 0; entity < g_world.entity_count; entity++) {
			if ((g_world.entity_signatures[entity] & system->signature) == system->signature) {
				relevant_entities[g_world.itr.count++] = entity;
			}
		}

		for (int comp = 0; comp < g_world.num_components; comp++) {
			if (system->signature & (1 << comp)) {
				u32 element_size = g_world.components[comp].element_size;
				void* view = malloc(g_world.itr.count * element_size);

				for (int entity_idx = 0; entity_idx < g_world.itr.count; entity_idx++) {
					Entity entity = relevant_entities[entity_idx];
					void* value = (u8*)g_world.components[comp].data + entity * element_size;
					memcpy((u8*)view + entity_idx * element_size, value, element_size);
				}
				g_world.itr.component_data[comp] = view;
			} else
				g_world.itr.component_data[comp] = NULL;
		}

		g_world.systems[sys].ptr(&g_world.itr);
		// TODO: Change to packed component arrays instead of temporary views
		for (int comp = 0; comp < g_world.num_components; comp++) {
			if (system->signature & (1 << comp)) {
				u32 element_size = g_world.components[comp].element_size;
				void* view = g_world.itr.component_data[comp];

				for (int entity_idx = 0; entity_idx < g_world.itr.count; entity_idx++) {
					Entity entity = relevant_entities[entity_idx];
					void* dst = (u8*)g_world.components[comp].data + entity * element_size;
					void* src = view + entity_idx * element_size;
					memcpy(dst, src, element_size);
				}
			}
			free(g_world.itr.component_data[comp]);
		}
	}
}
