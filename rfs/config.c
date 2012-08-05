#include "config.h"

#define WORKING_DIR         "/data/rfs/"
#define FILE_NAME_FORMAT    "rfs_$(file_type)_$(file_no)_$(grid_num)_$(grid_size).bin"

#if 0
    #define MAX_OPEN_FILE_NUM   (256)
    #define MAX_FILE_TYPE_NUM   (10)
    #define FILE_SIZE           (1024*1024*1024)
    #define BASE_FILE_GRID_SIZE (4*1024)
    #define GRID_SIZE_GROWTH_FACTOR (2)
    #define HASHTABLE_LIST_NUM  (1024*1024*80)
    #define HASHTABLE_NODE_NUM  (1024*1024*40)
#else
    #define MAX_OPEN_FILE_NUM   (3)
    #define MAX_FILE_TYPE_NUM   (2)
    #define FILE_SIZE           (4*256)
    #define BASE_FILE_GRID_SIZE (256)
    #define GRID_SIZE_GROWTH_FACTOR (2)
    #define HASHTABLE_LIST_NUM  (8)
    #define HASHTABLE_NODE_NUM  (4)
#endif

stSysConfig g_default_sys_config = {
    WORKING_DIR,
    FILE_NAME_FORMAT,
    MAX_FILE_TYPE_NUM,
    MAX_OPEN_FILE_NUM,
    FILE_SIZE,
    BASE_FILE_GRID_SIZE,
    GRID_SIZE_GROWTH_FACTOR,
    HASHTABLE_LIST_NUM,
    HASHTABLE_NODE_NUM,
};

stUserConfig g_default_user_config = {
    DELETE_OLD_DATA,
    1,
    1,
    1
};

