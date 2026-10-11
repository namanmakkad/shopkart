#include "hashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * DJB2 Hash Function:
 * Good distribution, simple bit shifts and addition.
 * Time Complexity: O(k) where k is key length.
 * Space Complexity: O(1).
 */
static unsigned long hash_function(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + (unsigned long)c; /* hash * 33 + c */
    }
    return hash;
}

void hashmap_init(HashMap *map) {
    if (!map) return;
    for (int i = 0; i < HASHMAP_BUCKETS; i++) {
        map->buckets[i] = NULL;
    }
    map->size = 0;
    map->collisions = 0;
}

int hashmap_put(HashMap *map, const char *key, const char *value, void *user_data) {
    if (!map || !key) return 0;

    unsigned long h = hash_function(key);
    int bucket = (int)(h % HASHMAP_BUCKETS);

    /* Check if key already exists to update value */
    HashEntry *cur = map->buckets[bucket];
    while (cur) {
        if (strncmp(cur->key, key, sizeof(cur->key)) == 0) {
            if (value) {
                snprintf(cur->value, sizeof(cur->value), "%s", value);
            } else {
                cur->value[0] = '\0';
            }
            cur->user_data = user_data;
            return 1;
        }
        cur = cur->next;
    }

    /* Key is new: allocate entry */
    HashEntry *entry = (HashEntry *)malloc(sizeof(HashEntry));
    if (!entry) return 0;

    snprintf(entry->key, sizeof(entry->key), "%s", key);
    if (value) {
        snprintf(entry->value, sizeof(entry->value), "%s", value);
    } else {
        entry->value[0] = '\0';
    }
    entry->user_data = user_data;

    /* If bucket already has elements, increment collision counter */
    if (map->buckets[bucket] != NULL) {
        map->collisions++;
    }

    /* Insert at head of separate chain: O(1) */
    entry->next = map->buckets[bucket];
    map->buckets[bucket] = entry;
    map->size++;
    return 1;
}

const char *hashmap_get(const HashMap *map, const char *key) {
    if (!map || !key) return NULL;

    unsigned long h = hash_function(key);
    int bucket = (int)(h % HASHMAP_BUCKETS);

    HashEntry *cur = map->buckets[bucket];
    while (cur) {
        if (strncmp(cur->key, key, sizeof(cur->key)) == 0) {
            return cur->value;
        }
        cur = cur->next;
    }
    return NULL;
}

void *hashmap_get_data(const HashMap *map, const char *key) {
    if (!map || !key) return NULL;

    unsigned long h = hash_function(key);
    int bucket = (int)(h % HASHMAP_BUCKETS);

    HashEntry *cur = map->buckets[bucket];
    while (cur) {
        if (strncmp(cur->key, key, sizeof(cur->key)) == 0) {
            return cur->user_data;
        }
        cur = cur->next;
    }
    return NULL;
}

int hashmap_remove(HashMap *map, const char *key) {
    if (!map || !key) return 0;

    unsigned long h = hash_function(key);
    int bucket = (int)(h % HASHMAP_BUCKETS);

    HashEntry *cur = map->buckets[bucket];
    HashEntry *prev = NULL;

    while (cur) {
        if (strncmp(cur->key, key, sizeof(cur->key)) == 0) {
            if (prev) {
                prev->next = cur->next;
            } else {
                map->buckets[bucket] = cur->next;
            }
            free(cur);
            map->size--;
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

int hashmap_size(const HashMap *map) {
    return map ? map->size : 0;
}

void hashmap_free(HashMap *map) {
    if (!map) return;
    for (int i = 0; i < HASHMAP_BUCKETS; i++) {
        HashEntry *cur = map->buckets[i];
        while (cur) {
            HashEntry *next = cur->next;
            free(cur);
            cur = next;
        }
        map->buckets[i] = NULL;
    }
    map->size = 0;
}
