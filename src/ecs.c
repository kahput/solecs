#include "ecs.h"
#include "defines.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_COMPONENTS 32
#define MAX_SYSTEMS 8

#define ENTITY_INDEX_BITS 24
#define ENTITY_GENERATION_BITS 8
#define ENTITY_INDEX_MASK ((1 << ENTITY_INDEX_BITS) - 1)
#define ENTITY_GENERATION_MASK ((1 << ENTITY_GENERATION_BITS) - 1)

typedef struct {
	void* data;
	size_t element_size;
	u32 count, capacity;

	// Packed component data
	u32* entity_to_index;
	Entity* index_to_entity;
} CompnentArray;

typedef struct {
	SystemPointer ptr;
	u32 signature, count;
} System;

typedef struct {
	CompnentArray components[MAX_COMPONENTS];
	System systems[MAX_SYSTEMS];
	u32 component_count, system_count;
	u32* entity_signatures;
	u8* entity_generations;
	u32 entity_count, entity_capcity;
	ECSIterator itr;
} ECS;

static ECS g_world = { 0 };

void ecs_startup(u32 component_count, ...) {
	assert(component_count <= MAX_COMPONENTS);
	g_world.component_count = component_count;
	g_world.system_count = g_world.entity_count = g_world.entity_capcity = 0;

	g_world.entity_signatures = NULL;

	g_world.itr = (ECSIterator){ 0 };
	g_world.itr.component_data = malloc(g_world.component_count * sizeof(void*));

	va_list args;
	va_start(args, component_count);
	for (int i = 0; i < component_count; i++) {
		g_world.components[i].element_size = va_arg(args, size_t);
		g_world.components[i].data = g_world.components[i].index_to_entity = g_world.components[i].entity_to_index = NULL;
		g_world.components[i].count = g_world.components[i].capacity = 0;
	}

	for (int i = 0; i < component_count; i++) {
		printf("INFO: COMPONENT: [%zuB] [ID %d] created.\n", g_world.components[i].element_size, i);
	}
	va_end(args);
}

void ecs_shutdown() {
	// Free component data
	for (int i = 0; i < g_world.component_count; i++) {
		free(g_world.components[i].data);
		free(g_world.components[i].entity_to_index);
		free(g_world.components[i].index_to_entity);
	}

	// Free entites
	free(g_world.entity_signatures);
	free(g_world.entity_generations);

	free(g_world.itr.component_data);
}

// Helper functions
u32 get_index(Entity entity) {
	return entity & ENTITY_INDEX_MASK;
}

u8 get_generation(Entity entity) {
	return (entity >> ENTITY_INDEX_BITS) & ENTITY_GENERATION_MASK;
}

bool is_entity_valid(Entity entity) {
	u32 index = get_index(entity);
	u8 generation = get_generation(entity);

	if (index >= g_world.entity_count)
		return false;
	if (generation != g_world.entity_generations[index])
		return false;

	return true;
}

// ECS managment
Entity create_entity() {
	if (g_world.entity_count == g_world.entity_capcity) {
		g_world.entity_capcity = g_world.entity_capcity ? g_world.entity_capcity * 2 : 32;
		g_world.entity_signatures = realloc(g_world.entity_signatures, g_world.entity_capcity * sizeof(u32));
		g_world.entity_generations = realloc(g_world.entity_generations, g_world.entity_capcity * sizeof(u8));
		memset(g_world.entity_generations + g_world.entity_count * sizeof(u8), 0, g_world.entity_capcity * sizeof(u8));

		printf("INFO: ENTITY: entity_signatures resized to %d.\n", g_world.entity_capcity);
	}

	Entity index = g_world.entity_count++;
	g_world.entity_signatures[index] = 0;
	if (g_world.entity_generations[index] == 0) {
		printf("INFO: ENTITY: Initializing generation of Entity [ID %d]\n", index);
		g_world.entity_generations[index] = 1;
	}
	printf("INFO: ENTITY: [ID %d] created. Entity count = %d\n", index, g_world.entity_count);
	return index | (1 << ENTITY_INDEX_BITS);
}

void destroy_entity(Entity entity) {
	if (!is_entity_valid(entity)) {
		printf("ERROR: ENTITY: Invalid entity [ID %d] in add_component().\n", get_index(entity));
		return;
	}
	u32 index = get_index(entity);
	u8 generation = get_generation(entity);

	g_world.entity_generations[index]++;
}

void add_component(Entity entity, u32 component_id, void* data) {
	assert(component_id < g_world.component_count);
	if (!is_entity_valid(entity)) {
		printf("ERROR: ENTITY: Invalid entity [ID %d] in add_component().\n", get_index(entity));
		return;
	}
	u32 index = get_index(entity);
	if ((g_world.entity_signatures[index] & (1 << component_id)) == (1 << component_id)) {
		printf("WARNING: ENTITY: Component[ID %d | %zuB] already added to entity [ID %d | Sig %d].\n", component_id, g_world.components[component_id].element_size, index, g_world.entity_signatures[index]);
		return;
	}

	CompnentArray* array = &g_world.components[component_id];
	size_t element_size = g_world.components[component_id].element_size;

	// TODO: Packed components
	if (index >= array->capacity) {
		array->capacity = array->capacity ? array->capacity * 2 < index ? index * 2 : array->capacity : 32;
		array->data = realloc(array->data, array->capacity * element_size);
		array->entity_to_index = realloc(array->entity_to_index, array->capacity * element_size);
		array->index_to_entity = realloc(array->index_to_entity, array->capacity * element_size);

		printf("INFO: COMPONENT: component_array[%d] resized to %d.\n", component_id, array->capacity);
	}

	g_world.entity_signatures[index] |= 1 << component_id;
	memcpy((u8*)array->data + index * element_size, data, element_size);
	printf("INFO: ENTITY: Component[ID %d | %zuB] added to entity [ID %d | Sig %d].\n", component_id, element_size, index, g_world.entity_signatures[index]);
	array->count++;
}

void remove_component(Entity entity, u32 component_id) {
	assert(component_id < g_world.component_count);
	if (!is_entity_valid(entity)) {
		printf("ERROR: ENTITY: Invalid entity [ID %d] in remove_component().\n", get_index(entity));
		return;
	}
	u32 index = get_index(entity);
	assert(index < g_world.entity_count);
	assert(component_id < g_world.component_count);
	CompnentArray* array = &g_world.components[component_id];
	array->count--;

	g_world.entity_signatures[index] &= ~(1 << component_id);
}

void* get_component(Entity entity, u32 component_id) {
	assert(component_id < g_world.component_count);
	if (!is_entity_valid(entity)) {
		printf("ERROR: ENTITY: Invalid entity [ID %d] in get_component().\n", get_index(entity));
		return NULL;
	}
	u32 index = get_index(entity);
	assert(index < g_world.entity_count);
	assert(component_id < g_world.component_count);
	CompnentArray* array = &g_world.components[component_id];
	size_t element_size = g_world.components[component_id].element_size;

	return ((u8*)array->data + index * element_size);
	return NULL;
}

void ecs_system(SystemPointer system_ptr, u32 component_count, ...) {
	assert(component_count <= MAX_COMPONENTS && g_world.system_count < MAX_SYSTEMS);
	System* system = &g_world.systems[g_world.system_count++];

	va_list args;
	va_start(args, component_count);
	for (int i = 0; i < component_count; i++) {
		system->signature |= 1 << va_arg(args, u32);
	}
	system->ptr = system_ptr;
	system->count = component_count;
	va_end(args);

	// TODO: Move out to update using updated_entities list

	printf("INFO: SYSTEM: System [Idx %d | Sig %d] added\n", (g_world.system_count - 1), system->signature);
}

void* ecs_field(ECSIterator* itr, u32 component_id) {
	return itr->component_data[component_id];
}

void ecs_update(f64 dt) {
	for (int sys = 0; sys < g_world.system_count; sys++) {
		System* system = &g_world.systems[sys];
		g_world.itr.count = 0;
		Entity relevant_entities[g_world.entity_count];

		// Check every entity signature against system signature.
		for (int entity = 0; entity < g_world.entity_count; entity++) {
			if ((g_world.entity_signatures[entity] & system->signature) == system->signature) {
				// printf("Entity[%d] added to system[%d]\n", entity, sys);
				relevant_entities[g_world.itr.count++] = entity;
			}
		}

		// Create a packed view of all relevant components of relevant entities
		for (int comp = 0; comp < g_world.component_count; comp++) {
			if (system->signature & (1 << comp)) {
				u32 element_size = g_world.components[comp].element_size;
				void* data_view = malloc(g_world.itr.count * element_size);

				for (int entity_idx = 0; entity_idx < g_world.itr.count; entity_idx++) {
					Entity entity = relevant_entities[entity_idx];
					entity |= (g_world.entity_generations[entity] << ENTITY_INDEX_BITS);
					void* dst = (u8*)data_view + entity_idx * element_size;
					void* src = get_component(entity, comp);
					memcpy(dst, src, element_size);
				}
				g_world.itr.component_data[comp] = data_view;
			} else
				g_world.itr.component_data[comp] = NULL;
		}

		g_world.systems[sys].ptr(&g_world.itr);
		// TODO: Change to packed component arrays instead of temporary views
		for (int comp = 0; comp < g_world.component_count; comp++) {
			if (system->signature & (1 << comp)) {
				u32 element_size = g_world.components[comp].element_size;
				void* view = g_world.itr.component_data[comp];

				for (int entity_idx = 0; entity_idx < g_world.itr.count; entity_idx++) {
					Entity entity = relevant_entities[entity_idx];
					entity |= (g_world.entity_generations[entity] << ENTITY_INDEX_BITS);
					void* dst = get_component(entity, comp);
					void* src = view + entity_idx * element_size;
					memcpy(dst, src, element_size);
				}
			}
			free(g_world.itr.component_data[comp]);
		}
	}
}
