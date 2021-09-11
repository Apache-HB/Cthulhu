#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

#define MAX(L, R) ((L) > (R) ? (L) : (R)) 
#define MIN(L, R) ((L) < (R) ? (L) : (R)) 

#define UNUSED(x) ((void)(x))

// memory managment

void *ctu_malloc(size_t size);
void *ctu_realloc(void *ptr, size_t size);
void ctu_free(void *ptr);
char *ctu_strdup(const char *str);
void init_memory(void);

// map collection

typedef struct entry_t {
    const char *key;
    void *value;
    struct entry_t *next;
} entry_t;

typedef struct {
    size_t size;
    entry_t data[];
} map_t;

typedef void(*map_apply_t)(void *user, void *value);
typedef bool(*map_collect_t)(void *value);
typedef void*(*vector_apply_t)(void *value);

map_t *map_new(size_t size);
void map_delete(map_t *map);
void *map_get(map_t *map, const char *key);
void map_set(map_t *map, const char *key, void *value);
void map_apply(map_t *map, void *user, map_apply_t func);

// vector collection

typedef struct {
    size_t size;
    size_t used;
    void *data[];
} vector_t;

vector_t *vector_new(size_t size);
vector_t *vector_of(size_t len);
vector_t *vector_init(void *value);
void vector_delete(vector_t *vector);
void vector_push(vector_t **vector, void *value);
void *vector_pop(vector_t *vector);
void vector_set(vector_t *vector, size_t index, void *value);
void *vector_get(const vector_t *vector, size_t index);
size_t vector_len(const vector_t *vector);
vector_t *vector_join(const vector_t *lhs, const vector_t *rhs);
vector_t *vector_map(const vector_t *vector, vector_apply_t func);

// queue collection

typedef struct {
    atomic_size_t size;
    atomic_size_t front;
    atomic_size_t back;
    void *data[];
} queue_t;

queue_t *queue_new(size_t size);
void queue_delete(queue_t *queue);
bool queue_write(queue_t *queue, void *value, bool blocking);
void *queue_read(queue_t *queue);
bool queue_is_empty(const queue_t *queue);

vector_t *map_collect(map_t *map, map_collect_t filter);

#define MAP_APPLY(map, user, func) map_apply(map, user, (map_apply_t)func)
#define MAP_COLLECT(map, filter) map_collect(map, (map_collect_t)filter)
#define VECTOR_MAP(vector, func) vector_map(vector, (vector_apply_t)func)
