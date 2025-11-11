#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "memory.h"
#include "object.h"
#include "utility.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

typedef struct {
    ObjectStr_t *key;
    Value_t value;
} Node_t;

typedef struct {
    int num_elems;
    int capacity;
    Node_t *table;
} HashTable_t;

void init_hash_table(HashTable_t *table);
void free_hash_table(HashTable_t *table);
bool insert(HashTable_t *hash_table, ObjectStr_t *key, Value_t value);
Value_t *get(HashTable_t *hash_table, ObjectStr_t *key);
bool drop(HashTable_t *hash_table, ObjectStr_t *key);
ObjectStr_t *find_str(HashTable_t *hash_table, const char *chars, int length, uint32_t hash);

#endif
