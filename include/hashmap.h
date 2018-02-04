#ifndef __HASHMAP_INC
#define __HASHMAP_INC

typedef struct _hashmap_entry {
    char *key;
    int value;
} hashmap_entry;

typedef struct _hashmap_entry_list {
    unsigned short vlen;
    unsigned short vroom;
    hashmap_entry *values;
} hashmap_entry_list;

typedef struct _hashmap {
    unsigned long num_entry;
    unsigned long bucketsize;
    hashmap_entry_list *buckets;
} hashmap;

typedef int (*hashmap_iterator)(void *context, const char *key, int value);

hashmap *hashmap_new();
void hashmap_put(hashmap *in, const char *key, int value);
int hashmap_remove(hashmap *in, const char *key);
int hashmap_get(hashmap *in, const char *key);
int hashmap_iterate(hashmap *in, hashmap_iterator iter, void *context);
void hashmap_empty(hashmap *in);
void hashmap_destroy(hashmap *in);
#endif
