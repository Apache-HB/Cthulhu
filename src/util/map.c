#include "cthulhu/util/map.h"
#include "cthulhu/util/str.h"

#include <stdint.h>

// generic map functions

NODISCARD
static size_t sizeof_map(size_t size)
{
    return sizeof(map_t) + (size * sizeof(bucket_t));
}

static bucket_t *bucket_new(const bucket_t *parent, alloc_t *alloc, const char *key, void *value)
{
    bucket_t *entry = alloc_new(alloc, sizeof(bucket_t), "bucket-new", parent);
    entry->key = key;
    entry->value = value;
    entry->next = NULL;
    return entry;
}

static bucket_t *get_bucket(map_t *map, size_t hash)
{
    size_t index = hash % map->size;
    bucket_t *entry = &map->data[index];
    return entry;
}

static void clear_keys(bucket_t *buckets, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        buckets[i].key = NULL;
        buckets[i].next = NULL;
    }
}

USE_DECL
map_t *map_new(size_t size, alloc_t *alloc)
{
    map_t *map = alloc_new(alloc, sizeof_map(size), "map-new", NULL);

    map->size = size;
    map->alloc = alloc;

    clear_keys(map->data, size);

    return map;
}

USE_DECL
vector_t *map_values(map_t *map)
{
    vector_t *result = vector_new(map->size);

    for (size_t i = 0; i < map->size; i++)
    {
        bucket_t *entry = &map->data[i];
        while (entry && entry->key)
        {
            vector_push(&result, entry->value);
            entry = entry->next;
        }
    }

    return result;
}

// string key map functions

static bucket_t *map_bucket_str(map_t *map, const char *key)
{
    size_t hash = strhash(key);
    return get_bucket(map, hash);
}

static void *entry_get(const bucket_t *entry, const char *key, void *other)
{
    if (entry->key && str_equal(entry->key, key))
    {
        return entry->value;
    }

    if (entry->next)
    {
        return entry_get(entry->next, key, other);
    }

    return other;
}

void *map_get_default(map_t *map, const char *key, void *other)
{
    bucket_t *bucket = map_bucket_str(map, key);
    return entry_get(bucket, key, other);
}

USE_DECL
void *map_get(map_t *map, const char *key)
{
    return map_get_default(map, key, NULL);
}

void map_set(map_t *map, const char *key, void *value)
{
    bucket_t *entry = map_bucket_str(map, key);

    while (true)
    {
        if (entry->key == NULL)
        {
            entry->key = key;
            entry->value = value;
            break;
        }

        if (str_equal(entry->key, key))
        {
            entry->value = value;
            break;
        }

        if (entry->next == NULL)
        {
            entry->next = bucket_new(entry, map->alloc, key, value);
            break;
        }

        entry = entry->next;
    }
}

// ptr map functions

static bucket_t *map_bucket_ptr(map_t *map, const void *key)
{
    size_t hash = ptrhash(key);
    return get_bucket(map, hash);
}

static void *entry_get_ptr(const bucket_t *entry, const void *key, void *other)
{
    if (entry->key == key)
    {
        return entry->value;
    }

    if (entry->next)
    {
        return entry_get_ptr(entry->next, key, other);
    }

    return other;
}

void map_set_ptr(map_t *map, const void *key, void *value)
{
    bucket_t *entry = map_bucket_ptr(map, key);

    while (true)
    {
        if (entry->key == NULL)
        {
            entry->key = key;
            entry->value = value;
            break;
        }

        if (entry->key == key)
        {
            entry->value = value;
            break;
        }

        if (entry->next == NULL)
        {
            entry->next = bucket_new(entry, map->alloc, key, value);
            break;
        }

        entry = entry->next;
    }
}

void *map_get_ptr(map_t *map, const void *key)
{
    return map_get_default_ptr(map, key, NULL);
}

void *map_get_default_ptr(map_t *map, const void *key, void *other)
{
    bucket_t *bucket = map_bucket_ptr(map, key);
    return entry_get_ptr(bucket, key, other);
}

static bucket_t *get_next_bucket(map_t *map, size_t *index, bucket_t *bucket)
{
    bucket_t *result = NULL;

    while (true)
    {
        if (bucket && bucket->next != NULL)
        {
            // if the chain has a bucket use that
            result = bucket->next;
        }
        else if (*index < map->size)
        {
            // otherwise go to the next element in the map if there is one
            result = &map->data[(*index)++];
        }

        if ((result != NULL && result->key != NULL) || *index >= map->size)
        {
            break;
        }
    }

    return result;
}

USE_DECL
map_iter_t map_iter(map_t *map)
{
    map_iter_t iter = {
        .map = map,
        .index = 0,
    };

    iter.bucket = get_next_bucket(map, &iter.index, NULL);
    iter.next = get_next_bucket(map, &iter.index, iter.bucket);

    return iter;
}

USE_DECL
map_entry_t map_next(map_iter_t *iter)
{
    map_entry_t entry = {
        .key = iter->bucket->key,
        .value = iter->bucket->value,
    };

    iter->bucket = iter->next;
    iter->next = get_next_bucket(iter->map, &iter->index, iter->bucket);

    return entry;
}

USE_DECL
bool map_has_next(map_iter_t *iter)
{
    return iter->bucket != NULL && iter->next != NULL;
}

void map_reset(map_t *map)
{
    clear_keys(map->data, map->size);
}
