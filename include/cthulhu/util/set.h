#pragma once

#include "macros.h"
#include <stddef.h>

/**
 * @brief a node in a chain of set entries
 */
typedef struct item_t {
    const char *key; ///< the key to this bucket
    struct item_t *next; ///< the next bucket in the chain
} item_t;

/**
 * @brief a hashset of strings
 */
typedef struct {
    size_t size; ///< the number of buckets
    item_t items[]; ///< the buckets
} set_t;

/**
 * @brief create a new set with a given number of buckets
 * 
 * @param size 
 * @return set_t* 
 */
set_t *set_new(size_t size);

/**
 * @brief delete a set
 * 
 * @param set the set to delete
 */
void set_delete(set_t *set);

/**
 * @brief add a string to a set
 * 
 * @param set the set to add to
 * @param key the key to add
 * @return a pointer to the deduplicated key
 */
const char* set_add(set_t *set, const char *key);

/**
 * @brief check if a set contains a key
 * 
 * @param set the set to check
 * @param key the key to check
 * @return true if the set contains the key
 */
bool set_contains(set_t *set, const char *key);
