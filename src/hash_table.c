#include "../includes/hash_table.h"
#include <stdint.h>

void init_hash_table(HashTable_t *hash_table) {
    hash_table->num_elems = 0;
    hash_table->capacity = 0;
    hash_table->table = NULL;
}

void free_hash_table(HashTable_t *hash_table) {
    free(hash_table->table);
    init_hash_table(hash_table);
}

static Node_t *find_insertion_slot(Node_t *table, ObjectStr_t *key, int capacity) {
    uint32_t idx = key->hash % capacity;
    Node_t *tombstone = NULL;
    for (int i = 0; i < capacity; i++) {
        Node_t *potential_slot = &table[idx];
        if (potential_slot->key == key) {
            return potential_slot;
        } else if (potential_slot->key == NULL) {
            // if already found tombstone earlier we want to return that instead
            if (IS_NONE_VAL(potential_slot->value)) {
                return tombstone ? tombstone : potential_slot;
            } else if (tombstone == NULL) {
                tombstone = potential_slot;
            }
        }
        idx = (idx + 1) % capacity;
    }
    return NULL;
}

static void resize_table(HashTable_t *hash_table, int new_capacity) {
    Node_t *new_table = (Node_t *)malloc(sizeof(Node_t) * new_capacity);
    if (new_table == NULL) {
        fprintf(stderr, "Error: not enough memory avaialable");
        return;
    }

    for (int i = 0; i < new_capacity; i++) {
        new_table[i].key = NULL;
        new_table[i].value = DECL_NONE_VAL;
    }

    hash_table->num_elems = 0;

    for (int i = 0; i < hash_table->capacity; i++) {
        Node_t *cur_slot = &hash_table->table[i];
        if (cur_slot->key == NULL) {
            continue;
        }
        Node_t *new_slot = find_insertion_slot(new_table, cur_slot->key, new_capacity);
        new_slot->key = cur_slot->key;
        new_slot->value = cur_slot->value;
        hash_table->num_elems++;
    }

    free(hash_table->table);
    hash_table->table = new_table;
    hash_table->capacity = new_capacity;
}

bool insert(HashTable_t *hash_table, ObjectStr_t *key, Value_t value) {
    if (hash_table->num_elems + 1 > hash_table->capacity * TABLE_MAX_LOAD) {
        int new_capacity = grow_capacity(hash_table->capacity);
        resize_table(hash_table, new_capacity);
    }

    Node_t *new_slot = find_insertion_slot(hash_table->table, key, hash_table->capacity);
    if (new_slot == NULL) {
        fprintf(stderr, "Error: hash table insertion failed");
        return false;
    }
    // if key already exist don't increase the element count
    bool res = new_slot->key == NULL;
    if (res && IS_NONE_VAL(new_slot->value)) {
        hash_table->num_elems++;
    }

    new_slot->key = key;
    new_slot->value = value;
    return res;
}

Value_t *get(HashTable_t *hash_table, ObjectStr_t *key) {
    if (hash_table->table == NULL) {
        return NULL;
    }

    Node_t *node = find_insertion_slot(hash_table->table, key, hash_table->capacity);
    if (node->key == NULL) {
        return NULL;
    }
    return &node->value;
}

bool drop(HashTable_t *hash_table, ObjectStr_t *key) {
    if (hash_table->table == NULL) {
        return false;
    }

    Node_t *node = find_insertion_slot(hash_table->table, key, hash_table->capacity);
    if (node->key == NULL) {
        return false;
    }

    node->key = NULL;
    node->value = DECL_BOOL_VAL(true);
    return true;
}

ObjectStr_t *find_str(HashTable_t *hash_table, const char *chars, int length, uint32_t hash) {
    if (hash_table->table == NULL) {
        return NULL;
    }
    uint32_t idx = hash % hash_table->capacity;
    for (int i = 0; i < hash_table->capacity; i++) {
        Node_t *node = &hash_table->table[idx];
        if (node->key == NULL && IS_NONE_VAL(node->value)) {
            return NULL;
        } else if (node->key->length == length && node->key->hash == hash &&
                   memcmp(node->key->chars, chars, length) == 0) {
            return node->key;
        }
        idx = (idx + 1) % hash_table->capacity;
    }
    return NULL;
}
