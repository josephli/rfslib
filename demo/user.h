#ifndef  INT_INC
#define  INT_INC

#include <stdint.h>
#include <string.h>
#include <stdio.h>

enum {
    TYPE_INT    = 1,
    TYPE_STRING = 2,
    TYPE_COUNT
};

uint32_t int_hash(void * key)
{
    return *(int *) key;
}

uint16_t int_type(void * key)
{
    return TYPE_INT;
}

int int_print(void * key, char * out)
{
    return sprintf(out, "%d", *(int *) key);
}

int int_cmp(void * key1, void * key2)
{
    return *(int *) key1 - *(int *) key2;
}

int int_serialize(void * key, char * value, uint16_t * len)
{
    memcpy(value, key, sizeof(int));
    *len = sizeof(int);

    return 0;
}

int int_deserialize(void * key, char * value, uint16_t len)
{
    memcpy(key, value, len);
    return 0;
}

uint32_t string_hash(void * key)
{
    uint32_t hash = 0;

    char * p = (char *) key;
    while (*p) hash = hash * 37 + *p++;

    return hash;
}

uint16_t string_type(void * key)
{
    return TYPE_STRING;
}

int string_print(void * key, char * out)
{
    return sprintf(out, "%s", (char *) key);
}

int string_cmp(void * key1, void * key2)
{
    char * p1 = (char *) key1;
    char * p2 = (char *) key2;

    return strncmp(p1, p2, strlen(p1));
}

int string_serialize(void * key, char * value, uint16_t * len)
{
    int klen = strlen((char *) key);
    if (klen > *len)
        return -1;

    *len = klen;
    strncpy(value, (char *) key, *len);

    return 0;
}

int string_deserialize(void * key, char * value, uint16_t len)
{
    memcpy(key, value, len);

    return 0;
}

#endif

