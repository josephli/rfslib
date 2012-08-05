#ifndef  HASH_TABLE_INC
#define  HASH_TABLE_INC

#include <stdint.h>

typedef struct {
    uint16_t file_type;
    uint16_t file_no;
} stFile;

typedef struct {
    stFile   file;
    uint32_t grid_idx;
} stIndex;

int64_t index_to_int64(stIndex * index);
int int64_to_index(int64_t i, stIndex * index);

typedef struct {
    uint32_t (* hash)        (void * key);
    uint16_t (* type)        (void * key);
    int      (* print)       (void * key, char * out);
    int      (* cmp)       (void * key1, void * key); 
    int      (* serialize)   (void * key, char * value, uint16_t * vlen); 
    int      (* deserialize) (void * key, char * value, uint16_t vlen); 
} stKeyCallback;

struct _stHashTable;
typedef struct _stHashTable stHashTable;

stHashTable * hashtable_create(uint32_t node_num, uint32_t list_num);
int hashtable_destroy(stHashTable * hash_table);
int hashtable_set(stHashTable * hash_table, void * key, stIndex * index, stKeyCallback * callback);
int hashtable_get(stHashTable * hash_table, void * key, stIndex * index, void **ctx, stKeyCallback * callback);
int hashtable_del(stHashTable * hash_table, void * key, stKeyCallback * callback);
int hashtable_next(stHashTable * hash_table, int32_t * idx, uint8_t * type, char *key, uint16_t * klen, stKeyCallback * callbacks);
int hashtable_print(stHashTable * hash_table, stKeyCallback * callbacks);

#endif

