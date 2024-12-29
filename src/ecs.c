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
} ComponentArray;

typedef struct {
	SystemPointer ptr;
	u32 signature, count;
} System;

typedef struct {
	ComponentArray components[MAX_COMPONENTS];
	System systems[MAX_SYSTEMS];
	u32 component_count, system_count;
	u32 *entity_signatures, *entity_to_index, *index_to_entity, *entity_free_ids;
	u32 entity_next_id, entity_count, entity_capcity, entity_free_ids_size;
} ECS;

static ECS g_world = { 0 };

void ecs_startup(u32 component_count, ...) {
	assert(component_count <= MAX_COMPONENTS);
	g_world.component_count = component_count;
	g_world.system_count = g_world.entity_next_id = g_world.entity_count = g_world.entity_capcity = g_world.entity_free_ids_size = 0;

	g_world.entity_signatures = g_world.index_to_entity = g_world.entity_to_index = g_world.entity_free_ids = NULL;

	va_list args;
	va_start(args, component_count);
	for (int i = 0; i < component_count; i++) {
		g_world.components[i].element_size = va_arg(args, size_t);
		g_world.components[i].data = NULL;
		g_world.components[i].count = g_world.components[i].capacity = 0;
		printf("INFO: COMPONENT: [ID %d | %zuB] Component defined.\n", i, g_world.components[i].element_size);
	}
	va_end(args);
}

void ecs_shutdown() {
	// Free component data
	for (int i = 0; i < g_world.component_count; i++)
		free(g_world.components[i].data);

	// Free entites
	free(g_world.entity_signatures);
	free(g_world.index_to_entity);
	free(g_world.entity_to_index);
	printf("INFO: ECS shutdown successfullly\n");
}

// Helper functions
u32 get_entity_id(Entity entity) {
	return entity & ENTITY_INDEX_MASK;
}

u8 get_entity_generation(Entity entity) {
	return (entity >> ENTITY_INDEX_BITS) & ENTITY_GENERATION_MASK;
}

bool is_entity_valid(Entity entity) {
	u32 entity_id = get_entity_id(entity);
	u32 entity_index = g_world.entity_to_index[entity_id];
	u8 entity_generation = get_entity_generation(entity);

	if (entity_index >= g_world.entity_count)
		return false;
	if (entity_generation != get_entity_generation(g_world.index_to_entity[entity_index]))
		return false;

	return true;
}

// ECS managment
Entity ecs_create_entity() {
	if (g_world.entity_count == g_world.entity_capcity) {
		g_world.entity_capcity = g_world.entity_capcity ? g_world.entity_capcity * 2 : 32;
		g_world.entity_signatures = realloc(g_world.entity_signatures, g_world.entity_capcity * sizeof(u32));
		g_world.index_to_entity = realloc(g_world.index_to_entity, g_world.entity_capcity * sizeof(u32));
		g_world.entity_to_index = realloc(g_world.entity_to_index, g_world.entity_capcity * sizeof(u32));
		g_world.entity_free_ids = realloc(g_world.entity_free_ids, g_world.entity_capcity * sizeof(u32));
		memset(g_world.index_to_entity + g_world.entity_count * sizeof(u32), 0, (g_world.entity_capcity - g_world.entity_count) * sizeof(u32));

		printf("INFO: ENTITY: Capacity resized to %d.\n", g_world.entity_capcity);
	}

	u32 entity_id, index;
	if (g_world.entity_free_ids_size > 0) {
		entity_id = g_world.entity_free_ids[--g_world.entity_free_ids_size];
		index = g_world.entity_count++;
	} else {
		entity_id = index = g_world.entity_count++;
	}

	u8 entity_generation = get_entity_generation(g_world.index_to_entity[index]);
	if (entity_generation == 0) {
		entity_generation = 1;
		printf("INFO: ENTITY: [ID %d] Entity generation initialized to 0\n", index);
	}
	g_world.entity_signatures[index] = 0;
	g_world.index_to_entity[index] = entity_id | (entity_generation << ENTITY_INDEX_BITS);
	g_world.entity_to_index[entity_id] = index;
	printf("INFO: ENTITY: [ID %d | IDX %d | GEN %d] Entity created\n", entity_id, index, entity_generation);
	printf("INFO: ENTITY: TOTAL: %d\n", g_world.entity_count);
	return g_world.index_to_entity[index];
}

void ecs_destroy_entity(Entity entity) {
	if (!is_entity_valid(entity)) {
		printf("WARNING: ENTITY: [ID %d] Invalid entity in ecs_destroy_entity()\n", get_entity_id(entity));
		return;
	}
	u32 deleted_entity_id = get_entity_id(entity);
	u32 deleted_entity_index = g_world.entity_to_index[deleted_entity_id];
	u32 deleted_entity_generation = get_entity_generation(entity);
	u32 last_index = --g_world.entity_count;

	printf("INFO: ENTITY: [ID %d | IDX %d | GEN %d] Entity marked for deletion\n", deleted_entity_id, deleted_entity_index, deleted_entity_generation);

	if (deleted_entity_index != last_index) {
		for (u32 comp = 0; comp < g_world.component_count; comp++) {
			size_t element_size = g_world.components[comp].element_size;
			void* component_data = g_world.components[comp].data;

			void* dst = (u8*)component_data + deleted_entity_index * element_size;
			void* src = (u8*)component_data + last_index * element_size;
			memcpy(dst, src, element_size);
		}

		Entity moved_entity = g_world.index_to_entity[last_index];
		u32 moved_entity_id = get_entity_id(moved_entity);
		u32 moved_entity_generation = get_entity_generation(moved_entity);
		g_world.entity_to_index[moved_entity_id] = deleted_entity_index;
		g_world.index_to_entity[deleted_entity_index] = moved_entity;
		g_world.entity_signatures[deleted_entity_index] = g_world.entity_signatures[last_index];
		printf("INFO: ENTITY: [ID %d | IDX %d | GEN %d] Entity moved to index %d\n",
				moved_entity_id,
				last_index,
				moved_entity_generation,
				g_world.entity_to_index[moved_entity_id]

		);
	}

	g_world.index_to_entity[last_index] = ENTITY_INDEX_MASK | (++deleted_entity_generation << ENTITY_INDEX_BITS);
	g_world.entity_to_index[deleted_entity_id] = UINT32_MAX;
	g_world.entity_signatures[last_index] = 0;
	g_world.entity_free_ids[g_world.entity_free_ids_size++] = deleted_entity_id;
	printf("INFO: ENTITY: [ID %d | IDX %d | GEN %d] Entity deleted\n", deleted_entity_id, deleted_entity_index, deleted_entity_generation);
}

void ecs_attach_component(Entity entity, u32 component_id, void* data) {
	assert(component_id < g_world.component_count);
	if (!is_entity_valid(entity)) {
		printf("WARNING: ENTITY: [ID %d] Invalid entity in ecs_attach_component()\n", get_entity_id(entity));
		return;
	}
	u32 entity_id = get_entity_id(entity);
	u32 index = g_world.entity_to_index[entity_id];

	if ((g_world.entity_signatures[index] & (1 << component_id)) == (1 << component_id)) {
		printf("WARNING: ENTITY: [ID %d | %zuB] Component already added to entity [ID %d]\n", component_id, g_world.components[component_id].element_size, index);
		return;
	}

	ComponentArray* array = &g_world.components[component_id];
	size_t element_size = g_world.components[component_id].element_size;

	if (array->count >= array->capacity) {
		array->capacity = array->capacity ? array->capacity * 2 : 32;
		array->data = realloc(array->data, array->capacity * element_size);

		printf("INFO: COMPONENT: [IDX %d] Component array resized to %d\n", component_id, array->capacity);
	}

	g_world.entity_signatures[index] |= 1 << component_id;
	memcpy((u8*)array->data + index * element_size, data, element_size);
	printf("INFO: COMPONENT: [ID %d | %zuB] Component added to entity [ID %d] at index %d\n", component_id, element_size, entity_id, index);
	array->count++;
}

void ecs_detach_component(Entity entity, u32 component_id) {
	assert(component_id < g_world.component_count);
	if (!is_entity_valid(entity)) {
		printf("WARNING: ENTITY: [ID %d] Invalid entity in ecs_detach_component()\n", get_entity_id(entity));
		return;
	}
	u32 entity_id = get_entity_id(entity);
	u32 index = g_world.entity_to_index[entity_id];
	assert(index < g_world.entity_count);
	assert(component_id < g_world.component_count);
	ComponentArray* array = &g_world.components[component_id];
	array->count--;

	g_world.entity_signatures[index] &= ~(1 << component_id);
}

void* ecs_fetch_component(Entity entity, u32 component_id) {
	assert(component_id < g_world.component_count);
	if (!is_entity_valid(entity)) {
		printf("WARNING: ENTITY: [ID %d] Invalid entity in ecs_fetch_component()\n", get_entity_id(entity));
		return NULL;
	}
	u32 entity_id = get_entity_id(entity);
	u32 index = g_world.entity_to_index[entity_id];
	assert(index < g_world.entity_count);
	assert(component_id < g_world.component_count);
	ComponentArray* array = &g_world.components[component_id];
	size_t element_size = g_world.components[component_id].element_size;

	return ((u8*)array->data + index * element_size);
	return NULL;
}

void ecs_attach_system(SystemPointer system_ptr, u32 component_count, ...) {
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

	printf("INFO: SYSTEM: [IDX %d | SIG %d] System added\n", (g_world.system_count - 1), system->signature);
}

void ecs_update(f64 dt) {
	for (int sys = 0; sys < g_world.system_count; sys++) {
		System* system = &g_world.systems[sys];

		// Check every entity signature against system signature.
		for (int index = 0; index < g_world.entity_count; index++) {
			if ((g_world.entity_signatures[index] & system->signature) == system->signature) {
				g_world.systems[sys].ptr(g_world.index_to_entity[index]);
			}
		}
	}
}
