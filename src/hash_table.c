#include <stdlib.h>

#include "bool.h"
#include "hash_table.h"
#include "string.h"

static unsigned long hash(char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++))
        h = ((h << 5) + h) + c;
    return h;
}

static Bool hash_table_resize(HashTable *table) {
    /* would be the head of each bucket in the for loop */
    Node *current;
    /* would be current->next */
    Node *next;
    /* new key index after resize */
    int new_index;
    /* old table size */
    int old_size = table->size;
    /* new table size */
    int new_size = old_size * 2;
    /* old table buckets array */
    Node **old_buckets = table->buckets;
    /* new table buckets array */
    Node **new_buckets;
    /* index tracker */
    int i;
    /* allocate buckets array, initialized to NULL */
    new_buckets = calloc(new_size, sizeof(Node *));
    /* if allocation failed, return false */
    if (!new_buckets)
        return false;
    /* a loop that goes through all buckets in old_buckets array for rehash */
    for (i = 0; i < old_size; i++) {
        /* assign the head node at buckets[i] to current */
        current = table->buckets[i];
        /* loop all nodes in buckets[i] */
        while (current) {
            /* rehash key with new_size */
            new_index = hash(current->key) % new_size;
            /* backup current->next before overwrite */
            next = current->next;
            /* overwrite current->next with new_buckets[new_index] */
            current->next = new_buckets[new_index];
            /* set the first Node in new_buckets[new_index] to current */
            new_buckets[new_index] = current;
            /* advance current to next (previously current->next) */
            current = next;
        }
    }
    /* overwrite table->buckets with new_buckets */
    table->buckets = new_buckets;
    /* overwrite table->size with new_size */
    table->size = new_size;
    /* free old_buckets array */
    free(old_buckets);
    return true;
}

HashTable *hash_table_create(void) {
    /* try to allocate new HashTable */
    HashTable *table = malloc(sizeof(HashTable));
    /* if malloc failed, return NULL */
    if (!table)
        return NULL;
    /* allocate buckets array, initialized to NULL */
    table->buckets = calloc(INITIAL_TABLE_SIZE, sizeof(Node *));
    /* if calloc failed, free table and return NULL */
    if (!table->buckets) {
        /* free table */
        free(table);
        return NULL;
    }
    /* set table size to INITIAL_TABLE_SIZE */
    table->size = INITIAL_TABLE_SIZE;
    /* set the count of key/value elements stored to 0 */
    table->count = 0;
    /* return table */
    return table;
}

Bool hash_table_insert(HashTable *table, char *key, void *data) {
    /* would be used as the new head if key doesn't exist in table */
    Node *node;
    /* get the index of key */
    int index = hash(key) % table->size;
    /* points to the head of the first node under buckets[index] */
    Node *current = table->buckets[index];
    /* runs as long as current is not NULL (each bucket can have other
     * keys aside of our key) */
    while (current) {
        /* if key is in table, update it and return true */
        if (strcmp(current->key, key) == 0) {
            /* overwrite current->data with our new data */
            current->data = data;
            return true;
        }
        /* current now points to next node (if any) */
        current = current->next;
    }
    /* allocate new node */
    node = malloc(sizeof(Node));
    /* if allocation failed, return false */
    if (!node)
        return false;
    /* allocate memory to clone our *char key to + 1 for NULL terminator */
    node->key = malloc(strlen(key) + 1);
    /* if allocation failed, free node and return false */
    if (!node->key) {
        /* free node */
        free(node);
        return false;
    }
    /* copy key to node->key */
    strcpy(node->key, key);
    /* assign node->data with our data */
    node->data = data;
    /* our new node's next points to the head of buckets[index] (can be NULL) */
    node->next = table->buckets[index];
    /* update the head of buckets[index] with our new node */
    table->buckets[index] = node;
    /* increment count for newly added node */
    table->count++;
    /* check if table needs resizing */
    if (table->count > table->size * 0.7)
        hash_table_resize(table); /* resize table */
    return true;
}

Bool hash_table_contains_key(HashTable *table, char *key) {
    /* get the index of key */
    int index = hash(key) % table->size;
    /* points to the head of the first node under buckets[index] */
    Node *current = table->buckets[index];
    /* as long as current is not NULL */
    while (current) {
        /* if key is in table, return its data */
        if (strcmp(current->key, key) == 0)
            return true;
        /* current now points to next node (if any) */
        current = current->next;
    }
    /* return NULL because key not found in table */
    return false;
}

void *hash_table_lookup(HashTable *table, char *key) {
    /* get the index of key */
    int index = hash(key) % table->size;
    /* points to the head of the first node under buckets[index] */
    Node *current = table->buckets[index];
    /* as long as current is not NULL */
    while (current) {
        /* if key is in table, return its data */
        if (strcmp(current->key, key) == 0)
            return current->data;
        /* current now points to next node (if any) */
        current = current->next;
    }
    /* return NULL because key not found in table */
    return NULL;
}

void hash_table_foreach(HashTable *table, void (*callback)(char *key, void *data, void *context), void *context) {
    /* index tracker */
    int i;
    /* would be the current node in the bucket chain */
    Node *node;
    /* a loop that goes through all buckets */
    for (i = 0; i < table->size; i++) {
        /* loop all nodes in buckets[i] */
        for (node = table->buckets[i]; node != NULL; node = node->next) {
            /* call the callback function with key, data, and user provided context */
            callback(node->key, node->data, context);
        }
    }
}

void hash_table_free(HashTable *table, void (*free_data)(void *)) {
    /* would be the head of each bucket in the for loop */
    Node *current;
    /* would be current->next */
    Node *next;
    /* index tracker */
    int i;
    /* a loop that goes through all buckets */
    for (i = 0; i < table->size; i++) {
        /* assign the head node at buckets[i] to current */
        current = table->buckets[i];
        /* loop as long as current is not NULL and frees data (if free_data was
         * provided), key, and the node itself */
        while (current) {
            /* if free_data provided, call it to free data */
            if (free_data)
                free_data(current->data);
            /* free key */
            free(current->key);
            /* backup current->next before freeing current */
            next = current->next;
            /* free current */
            free(current);
            /* assign next to current */
            current = next;
        }
    }
    /* free the whole buckets array */
    free(table->buckets);
    /* at last, free table */
    free(table);
}