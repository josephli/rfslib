#include "hash_table.h"
#include "singly_list.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

int64_t index_to_int64(stIndex * index)
{
    return ((index->file.file_type << 24) + (index->file.file_no << 16) + (index->grid_idx));
}

int int64_to_index(int64_t i, stIndex * index)
{
    index->file.file_type = (i & 0xFF000000) >> 24;
    index->file.file_no   = (i & 0x00FF0000) >> 16;
    index->grid_idx       = i & 0x0000FFFF;

    return 0;
}

typedef struct {
    uint8_t  type;
    uint16_t len;
    char key[MAX_KEY_LEN+1];
} stKey;

typedef struct _stNode {
    struct _stNode * prec;
    struct _stNode * next;
    stKey   key;
    stIndex value;
} stNode;

typedef struct {
    stNode * head;
} stLinkList ;

typedef struct {
    uint32_t node_num;
    stSinglyList * list;
    stNode * nodes;
} stNodePool;

struct _stHashTable {
    uint32_t     list_num;
    stLinkList * lists;
    stNodePool * pool;
    char         private_data[MAX_KEY_LEN+1];
};

static stNode * _pool_peek_idle_node(stNodePool * pool)
{
    int idle_idx = sl_peek_idle_idx(pool->list);
    if (idle_idx < 0)
        return NULL;

    assert(idle_idx < pool->node_num);
    return pool->nodes + idle_idx;
}

static int _pool_consume_idle_node(stNodePool * pool, stNode * node)
{
    int idx = node - pool->nodes;
    if (idx < 0 || idx > pool->node_num)
        return -1;

    return sl_consume_idle_idx(pool->list, idx);
}

static int _pool_recycle_idle_node(stNodePool * pool, stNode * node)
{
    int idx = node - pool->nodes;
    if (idx < 0 || idx > pool->node_num)
        return -1;

    node->prec = NULL;
    node->next = NULL;
    node->key.type = 0;
    node->key.len = MAX_KEY_LEN;

    return sl_recycle_used_idx(pool->list, idx);
}

stHashTable * hashtable_create(uint32_t node_num, uint32_t list_num)
{
    stHashTable * hash_table  = calloc(1, sizeof(stHashTable));

    hash_table->list_num = list_num;
    hash_table->lists = calloc(list_num, sizeof(stLinkList));
    hash_table->pool  = calloc(1, sizeof(stNodePool));

    stNodePool * pool = hash_table->pool;
    pool->node_num = node_num;
    pool->list  = sl_create(pool->node_num);
    pool->nodes = calloc(pool->node_num, sizeof(stNode));

    return hash_table;
}

int hashtable_destroy(stHashTable * hash_table)
{
    assert(hash_table != NULL);

    free(hash_table->lists);
    sl_destroy(hash_table->pool->list);
    free(hash_table->pool->nodes);
    free(hash_table->pool);
    free(hash_table);

    return 0;
}

int hashtable_get(stHashTable * hash_table, void * key, stIndex * index, void **ctx, stKeyCallback * callback)
{
    uint32_t hash = (callback->hash(key) % hash_table->list_num);
    stLinkList * p = hash_table->lists + hash;

    stNode * node = p->head;
    for (; node != NULL; node = node->next)
    {
        if (node->key.type != callback->type(key))
            continue;

        if (callback->deserialize(hash_table->private_data, node->key.key, node->key.len) != 0)
            continue;

        if (callback->cmp(hash_table->private_data, key) != 0)
            continue;

        if (index != NULL) *index = node->value;
        if (ctx != NULL) *ctx = node;

        return 0;
    }

    return -1;
}

int hashtable_set(stHashTable * hash_table, void * key, stIndex * index, stKeyCallback * callback)
{
    stNode *node = NULL;
    if (hashtable_get(hash_table, key, NULL, (void **)&node, callback) == 0)
    {
        node->value = *index;
        return 0;
    }

    assert(node == NULL);

    node = _pool_peek_idle_node(hash_table->pool);
    if (node == NULL)
        return -1;

    node->key.len = MAX_KEY_LEN;
    int ret = callback->serialize(key, node->key.key, &node->key.len);
    if (ret != 0)
        return -1;

    node->key.type = callback->type(key);
    node->value = *index;

    uint32_t hash  = (callback->hash(key) % hash_table->list_num);
    stLinkList * p = hash_table->lists + hash;

    node->prec = NULL;
    node->next = p->head;
    if (p->head != NULL) p->head->prec = node;
    p->head = node;

    return _pool_consume_idle_node(hash_table->pool, node);
}

int hashtable_del(stHashTable * hash_table, void * key, stKeyCallback * callback)
{
    stNode *node;
    if (hashtable_get(hash_table, key, NULL, (void **)&node, callback) == 0)
    {
        assert(node != NULL);
        stNode * prec = node->prec;
        stNode * next = node->next;
        if (prec != NULL) prec->next = next;
        if (next != NULL) next->prec = prec;

        uint32_t hash  = (callback->hash(key) % hash_table->list_num);
        stLinkList * p = hash_table->lists + hash;
        if (p->head == node) p->head = next;

        return _pool_recycle_idle_node(hash_table->pool, node);
    }

    return -1;
}

int hashtable_next(stHashTable * hash_table, int32_t * idx, uint8_t * type, char *key, uint16_t * klen, stKeyCallback * callbacks)
{
    stNodePool * pool = hash_table->pool;

    int32_t i = *idx + 1;
    for (; i < pool->node_num; ++i)
    {
        stNode * node = pool->nodes + i;

        if (node->key.type == 0)
            continue;

        *type = node->key.type;
        if (callbacks[*type].deserialize(key, node->key.key, node->key.len) != 0)
            continue;

        *idx  = i;
        *klen = node->key.len;
        return 0;
    }

    return -1;
}

int hashtable_print(stHashTable * hash_table, stKeyCallback * callbacks)
{
    uint8_t printed = 0;
    uint32_t i = 0;
    for (; i < hash_table->list_num; ++i)
    {
        printed = 0;
        stLinkList * p = hash_table->lists + i;

        stNode * node = p->head;
        for (; node != NULL; node = node->next)
        {
            if (node->key.type == 0)
                continue;

            if (callbacks[node->key.type].deserialize(hash_table->private_data, node->key.key, node->key.len) != 0)
                continue;

            char out[MAX_KEY_LEN+1] = {0};
            callbacks[node->key.type].print(hash_table->private_data, out);

            if (printed == 0)
            {
                printf("hash collision at %u:\n", i);
                printed = 1;
            }

            printf("\tkey %s stored at file_type: %hu, file_no: %hu, grid_idx: %u\n", 
                out, node->value.file.file_type, node->value.file.file_no, node->value.grid_idx);
        }
    }

    return -1;
}

