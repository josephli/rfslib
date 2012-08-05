#ifndef  RFS_INC
#define  RFS_INC

#include <stdint.h>
#include "config.h"
#include "hash_table.h"

struct _rfs;
typedef struct _rfs rfs;

rfs * rfs_create(stSysConfig sys_config, stUserConfig user_config, uint8_t type_count, stKeyCallback* user_callbacks);
int rfs_destroy(rfs * pfs);

//返回编码: [63-0]共64位,[63-48]:file_type,[47-32]:file_no,[31-0]:grid_idx
//返回-1表示失败
int64_t rfs_get(rfs * pfs, uint8_t type, void * key, char * value, uint16_t * vlen, char * info, uint16_t ilen);

//返回编码: [63-0]共64位,[63-48]:file_type,[47-32]:file_no,[31-0]:grid_idx
//返回-1表示失败
int64_t rfs_set(rfs * pfs, uint32_t now, uint8_t type, void * key, char * value, uint16_t vlen, char * info, uint16_t ilen);

int rfs_del(rfs * pfs, uint8_t type, void * key, char * info, uint16_t ilen);

int rfs_print_data(rfs * pfs);
int rfs_print_hashtable(rfs * pfs);

#endif

