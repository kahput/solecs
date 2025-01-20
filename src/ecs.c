#include "ecs.h"
#include "defines.h"

#include <cstddef>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define INVALID_INDEX -1

#define MAX_COMPONENTS 32

#define ENTITY_INDEX_BITS 24
#define ENTITY_GENERATION_BITS 8
#define ENTITY_INDEX_MASK ((1 << ENTITY_INDEX_BITS) - 1)
#define ENTITY_GENERATION_MASK ((1 << ENTITY_GENERATION_BITS) - 1)

typedef struct _component_array {
	void* data;
	size_t element_size;
	u32 count, capacity;
} ComponentArray;

typedef struct _archetype {
	ComponentArray components[MAX_COMPONENTS];
	u32 *index_to_eid, *eid_to_index;
	u32 signature, entity_count, component_count;
	struct _archetype* next;
} Archetype;

struct _world {
	Archetype** archetypes;
	u32 archetype_count, archetype_capacity;
	u32 *entity_signatures, *entity_generations, *entity_recyclable;
	u32 entity_count, entity_capcity, entity_next_id, recyclable_count;
};

// Helper functions
void* resize_array(void* data, u32 old_capacity, u32 new_capacity, u32 element_size) {
	void* tmp = realloc(data, new_capacity * element_size);
	if (!tmp) {
		printf("Realloc failed at %s:%d\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE); // Or handle error
	}
	memset((u8*)tmp + (old_capacity * element_size), 0, (new_capacity - old_capacity) * element_size);
	return tmp;
}

u32 get_entity_id(Entity entity) {
	return entity & ENTITY_INDEX_MASK;
}

u8 get_entity_generation(Entity entity) {
	return (entity >> ENTITY_INDEX_BITS) & ENTITY_GENERATION_MASK;
}

u32 hash(u32 signature, size_t hash_map_capacity) {
	signature = ((signature >> 16) ^ signature) * 0x45d9f3b;
	signature = ((signature >> 16) ^ signature) * 0x45d9f3b;
	signature = (signature >> 16) ^ signature;
	return signature & (hash_map_capacity - 1);
}

Archetype* signature_to_archetype(World* world, u32 signature) {
	u32 index = hash(signature, world->archetype_capacity);
	Archetype* current = world->archetypes[index];
	while (current) {
		if (current->signature == signature)
			return current;

		current = current->next;
	}
	return NULL;
}

void add_archetype(World* world, Archetype* archetype) {
	u32 index = hash(archetype->signature, world->archetype_capacity);
	archetype->next = world->archetypes[index];
	world->archetypes[index] = archetype;
}

Archetype* fetch_or_create_archetype(World* world, u32 signature) {
	Archetype* archetype = signature_to_archetype(world, signature);
	if (archetype)
		return archetype;

	archetype = calloc(1, sizeof(Archetype));
	archetype->signature = signature;
	add_archetype(world, archetype);
	return archetype;
}

World* ecs_startup() {
	World* world = malloc(sizeof(World));
	world->archetypes = NULL;
	world->archetype_count = world->entity_count = world->entity_next_id = world->recyclable_count = 0;
	world->archetype_capacity = world->entity_capcity = 0;
	world->entity_signatures = world->entity_generations = world->entity_recyclable = NULL;
	return world;
}

void ecs_shutdown(World* world) {
	for (int i = 0; i < world->archetype_count; i++) {
		Archetype* current = world->archetypes[i];
		Archetype* prev = NULL;
		while (current) {
			for (int j = 0; j < current->component_count; j++) {
				free(current->components[j].data);
			}
			free(current->eid_to_index);
			free(current->index_to_eid);
			prev = current;
			current = current->next;
			free(prev);
		}
	}
	free(world->entity_signatures);
	free(world->entity_generations);
	free(world->entity_recyclable);
}

void ecs_destroy_entity(World* world, Entity entity);
bool ecs_validate_entity(World* world, Entity entity);

void ecs_register_component(World* world, Entity entity) {
}
void ecs_attach_component(World* world, Entity entity, u32 component_id, void* data);
void ecs_detach_component(World* world, Entity entity, u32 component_id);
void* ecs_fetch_component(World* world, Entity entity, u32 component_id) {
	u32 signautre = (1 << component_id);
	if ((world->entity_signatures[entity] & signautre) != signautre)
		return NULL;

	Archetype* at = signature_to_archetype(world, signautre);
	u32 index = at->eid_to_index[get_entity_id(entity)];
	ComponentArray* array = &at->components[component_id];
	return array->data + index * array->element_size;
}
bool ecs_has_component(World* world, Entity entity, u32 component_id);

void ecs_query(u32 component_count, ...);

void ecs_update(f64 dt);
