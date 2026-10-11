#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>

#define HASHMAP_BUCKETS 128
#define HASHMAP_KEY_LEN 128
#define HASHMAP_VAL_LEN 512

/*
 * Hash Table entry using separate chaining for collision resolution.
 */
typedef struct HashEntry {
    char key[HASHMAP_KEY_LEN];
    char value[HASHMAP_VAL_LEN];
    void *user_data; /* optional custom pointer */
    struct HashEntry *next;
} HashEntry;

/*
 * Hash Table struct with separate chaining.
 * Time Complexity: O(1) average lookup/insert/delete, O(n) worst case with all collisions.
 * Space Complexity: O(b + n) where b is bucket count and n is entry count.
 */
typedef struct {
    HashEntry *buckets[HASHMAP_BUCKETS];
    int size;
    unsigned long long collisions;
} HashMap;

/* Initialize hash map */
void hashmap_init(HashMap *map);

/* Insert or update key-value pair. Returns 1 on success, 0 on memory error. */
int hashmap_put(HashMap *map, const char *key, const char *value, void *user_data);

/* Lookup value by key. Returns value string or NULL if not found. */
const char *hashmap_get(const HashMap *map, const char *key);

/* Lookup custom user_data by key. Returns pointer or NULL. */
void *hashmap_get_data(const HashMap *map, const char *key);

/* Remove key from map. Returns 1 if removed, 0 if not found. */
int hashmap_remove(HashMap *map, const char *key);

/* Get number of entries */
int hashmap_size(const HashMap *map);

/* Free all nodes in hash map */
void hashmap_free(HashMap *map);

#endif /* HASHMAP_H */
