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

	// Entity ID to Index, Endex to EntityID
	u32 *eid_index, *index_eid;
} ComponentArray;

typedef struct {
	SystemPointer ptr;
	u32 required_component_id, signature;
} System;

typedef struct {
	ComponentArray components[MAX_COMPONENTS];
	System systems[MAX_SYSTEMS];
	u32 component_count, system_count;
	u32 *entity_signatures, *entity_generations, *entity_free_ids;
	u32 entity_next_id, entity_count, entity_capcity, entity_free_ids_size;
} ECS;

static ECS g_world = { 0 };

void ecs_startup(u32 component_count, ...) {
	assert(component_count <= MAX_COMPONENTS);
	g_world.component_count = component_count;
	g_world.system_count = g_world.entity_next_id = g_world.entity_count = g_world.entity_capcity = g_world.entity_free_ids_size = 0;

	g_world.entity_signatures = g_world.entity_free_ids = NULL;

	va_list args;
	va_start(args, component_count);
	for (int i = 0; i < component_count; i++) {
		g_world.components[i].element_size = va_arg(args, size_t);
		g_world.components[i].data = g_world.components[i].eid_index = g_world.components[i].index_eid = NULL;
		g_world.components[i].count = g_world.components[i].capacity = 0;
		printf("INFO: COMPONENT: [ID %d | %zuB] Component defined.\n", i, g_world.components[i].element_size);
	}
	va_end(args);
}

void ecs_shutdown() {
	// Free component data
	for (int i = 0; i < g_world.component_count; i++) {
		if (!g_world.components[i].count)
			continue;
		free(g_world.components[i].data);
		free(g_world.components[i].eid_index);
		free(g_world.components[i].index_eid);
	}

	// Free entites
	free(g_world.entity_signatures);
	free(g_world.entity_free_ids);
	printf("INFO: ECS shutdown successfullly\n");
}

// Helper functions
u32 get_entity_id(Entity entity) {
	return entity & ENTITY_INDEX_MASK;
}

u8 get_entity_generation(Entity entity) {
	return (entity >> ENTITY_INDEX_BITS) & ENTITY_GENERATION_MASK;
}

void* resize_array(void* data, u32 old_capacity, u32 new_capacity, u32 element_size) {
	void* tmp = realloc(data, new_capacity * element_size);
	if (!tmp) {
		printf("Realloc failed at %s:%d\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE); // Or handle error
	}
	memset((u8*)tmp + (old_capacity * element_size), 0, (new_capacity - old_capacity) * element_size);
	return tmp;
}

// ECS managment
Entity ecs_create_entity() {
	if (g_world.entity_count == g_world.entity_capcity) {
		g_world.entity_capcity = g_world.entity_capcity ? g_world.entity_capcity * 2 : 32;
		g_world.entity_signatures = (u32*)resize_array(g_world.entity_signatures, g_world.entity_count, g_world.entity_capcity, sizeof(u32));
		g_world.entity_generations = (u32*)resize_array(g_world.entity_generations, g_world.entity_count, g_world.entity_capcity, sizeof(u32));
		g_world.entity_free_ids = (u32*)resize_array(g_world.entity_free_ids, g_world.entity_count, g_world.entity_capcity, sizeof(u32));

		printf("INFO: ENTITY: Capacity resized to %d.\n", g_world.entity_capcity);
	}

	u32 entity_id = g_world.entity_free_ids_size ? g_world.entity_free_ids[--g_world.entity_free_ids_size] : g_world.entity_count++;
	u8 entity_generation = g_world.entity_generations[entity_id];
	g_world.entity_signatures[entity_id] = 0;

	printf("INFO: ENTITY: [ID %d | GEN %d] Entity created\n", entity_id, entity_generation);
	printf("INFO: ENTITY: TOTAL: %d\n", g_world.entity_count);
	return entity_id | (entity_generation << ENTITY_INDEX_BITS);
}

void ecs_destroy_entity(Entity entity) {
	if (!ecs_validate_entity(entity)) {
		printf("WARNING: ENTITY: [ID %d] Invalid entity in ecs_destroy_entity()\n", get_entity_id(entity));
		return;
	}
	u32 deleted_entity_id = get_entity_id(entity);
	u32 deleted_entity_generation = get_entity_generation(entity);

	printf("INFO: ENTITY: [ID %d | GEN %d] Entity marked for deletion\n", deleted_entity_id, deleted_entity_generation);

	for (u32 comp = 0; comp < g_world.component_count; comp++) {
		if (!((g_world.entity_signatures[deleted_entity_id] & (1 << comp)) == (1 << comp)))
			continue;
		ecs_detach_component(deleted_entity_id, comp);
	}
	g_world.entity_generations[deleted_entity_id] = ++deleted_entity_generation;
	g_world.entity_signatures[deleted_entity_id] = 0;
	g_world.entity_free_ids[g_world.entity_free_ids_size++] = deleted_entity_id;
	printf("INFO: ENTITY: [ID %d | GEN %d] Entity deleted\n", deleted_entity_id, deleted_entity_generation);
}

bool ecs_validate_entity(Entity entity) {
	u32 entity_id = get_entity_id(entity);
	u8 entity_generation = get_entity_generation(entity);

	if (entity_generation != g_world.entity_generations[entity_id])
		return false;

	return true;
}

void ecs_attach_component(Entity entity, u32 component_id, void* data) {
	assert(component_id < g_world.component_count);
	if (!ecs_validate_entity(entity)) {
		printf("WARNING: ENTITY: [ID %d] Invalid entity in ecs_attach_component()\n", get_entity_id(entity));
		return;
	}
	u32 entity_id = get_entity_id(entity);

	if ((g_world.entity_signatures[entity_id] & (1 << component_id)) == (1 << component_id)) {
		printf("WARNING: ENTITY: [ID %d | %zuB] Component already added to entity [ID %d]\n", component_id, g_world.components[component_id].element_size, entity_id);
		return;
	}

	ComponentArray* array = &g_world.components[component_id];

	if (array->count >= array->capacity) {
		array->capacity = array->capacity ? array->capacity * 2 : 32;
		array->data = resize_array(array->data, array->count, array->capacity, array->element_size);
		array->eid_index = resize_array(array->eid_index, array->count, array->capacity, array->element_size);
		array->index_eid = resize_array(array->index_eid, array->count, array->capacity, array->element_size);

		printf("INFO: COMPONENT: [IDX %d] Component array resized to %d\n", component_id, array->capacity);
	}

	g_world.entity_signatures[entity_id] |= 1 << component_id;
	memcpy((u8*)array->data + array->count * array->element_size, data, array->element_size);
	array->index_eid[array->count] = entity_id;
	array->eid_index[entity_id] = array->count;
	printf("INFO: COMPONENT: [ID %d | %zuB] Component added to entity [ID %d] at index %d\n", component_id, array->element_size, entity_id, array->count);
	array->count++;
}

void ecs_detach_component(Entity entity, u32 component_id) {
	assert(component_id < g_world.component_count);
	if (!ecs_validate_entity(entity)) {
		printf("WARNING: ENTITY: [ID %d] Invalid entity in ecs_detach_component()\n", get_entity_id(entity));
		return;
	}
	ComponentArray* array = &g_world.components[component_id];
	u32 entity_id = get_entity_id(entity);
	u32 index = array->eid_index[entity_id];
	u32 last_index = --array->count;
	u32 last_eid = array->index_eid[last_index];

	if (index != last_index) {
		void* dst = (u8*)array->data + index * array->element_size;
		void* src = (u8*)array->data + last_index * array->element_size;
		memcpy(dst, src, array->element_size);
	}

	array->index_eid[index] = last_eid;
	array->eid_index[last_eid] = index;

	g_world.entity_signatures[entity_id] &= ~(1 << component_id);
}

void* ecs_fetch_component(Entity entity, u32 component_id) {
	assert(component_id < g_world.component_count);
	if (!ecs_validate_entity(entity)) {
		printf("WARNING: ENTITY: [ID %d] Invalid entity in ecs_fetch_component()\n", get_entity_id(entity));
		return NULL;
	}
	ComponentArray* array = &g_world.components[component_id];
	u32 entity_id = get_entity_id(entity);
	u32 index = array->eid_index[entity_id];

	return ((u8*)array->data + index * array->element_size);
}

void ecs_attach_system(SystemPointer system_ptr, u32 component_count, ...) {
	assert(component_count <= MAX_COMPONENTS && g_world.system_count < MAX_SYSTEMS);
	System* system = &g_world.systems[g_world.system_count++];
	system->required_component_id = -1;

	va_list args;
	va_start(args, component_count);
	for (int i = 0; i < component_count; i++) {
		u32 component_id = va_arg(args, u32);
		if (system->required_component_id == -1)
			system->required_component_id = component_id;
		system->signature |= 1 << component_id;
	}
	system->ptr = system_ptr;
	va_end(args);

	printf("INFO: SYSTEM: [IDX %d | SIG %d] System added\n", (g_world.system_count - 1), system->signature);
}

void* primary(u32 primary_id, u32 index) {
	ComponentArray* array = &g_world.components[primary_id];
	// printf("Primary ID: %d, Index: %d, array_count: %d, element_size: %zu\n", primary_id, index, array->count, array->element_size);
	return ((u8*)array->data + index * array->element_size);
}

void* secondary(u32 primary_id, u32 index, u32 component_id) {
	assert(g_world.component_count > primary_id && g_world.component_count > component_id);
	ComponentArray* array = &g_world.components[primary_id];
	ComponentArray* other = &g_world.components[component_id];
	u32 other_index = other->eid_index[array->index_eid[index]];
	return ((u8*)other->data + other_index * other->element_size);
}

void ecs_update(f64 dt) {
	for (int sys = 0; sys < g_world.system_count; sys++) {
		System* system = &g_world.systems[sys];
		ComponentArray* array = &g_world.components[system->required_component_id];
		Query q = {
			.id = system->required_component_id,
			.count = array->count,
			.primary = primary,
			.secondary = secondary
		};
		system->ptr(&q);
	}
}
