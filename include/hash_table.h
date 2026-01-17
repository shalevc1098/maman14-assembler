/* include guard to define only once */
#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "bool.h"

#define INITIAL_TABLE_SIZE 8 /* the initial size that newly created tables would start from */

/* generic node - stores key + any data */
typedef struct Node {
    char *key;
    void *data;        /* can point to anything */
    struct Node *next; /* for chaining */
} Node;

/* hash table */
typedef struct {
    Node **buckets; /* array of buckets */
    int size;       /* table size */
    int count;      /* number of key/value pairs stored */
} HashTable;

/* creates new table */
HashTable *hash_table_create(void);
/* inserts data into table */
Bool hash_table_insert(HashTable *table, char *key, void *data);
/* checks if table contains key */
Bool hash_table_contains_key(HashTable *table, char *key);
/* lookups for key in table and returns its data */
void *hash_table_lookup(HashTable *table, char *key);
/* loop all data in table and call callback on them */
void hash_table_foreach(HashTable *table, void (*callback)(char *key, void *data, void *context), void *context);
/* frees table */
void hash_table_free(HashTable *table, void (*free_data)(void *));

#endif