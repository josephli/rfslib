typedef struct 
{
    const char *name;
    const char *help;
    int (*handle_func)(int argc, char *argv[]);
} cmd_t;

static cmd_t *get_cmd(const char *name);
#define DIM(x) (sizeof(x) / sizeof((x)[0]))

//user cmd

#include "rfs.h"
#include "user.h"
#include <stdlib.h>

stKeyCallback user_callbacks[TYPE_COUNT] = {
    {NULL, NULL, NULL, NULL, NULL, NULL},
    {int_hash, int_type, int_print, int_cmp, int_serialize, int_deserialize},
    {string_hash, string_type, string_print, string_cmp, string_serialize, string_deserialize}
};

static int set_int(int argc, char **argv)
{
    if (argc != 3)
    {
        cmd_t *cmd = get_cmd(__FUNCTION__);
        printf("format error. \n%s %s\n", cmd->name, cmd->help);
        return -1;
    }

    rfs * pfs = rfs_create(g_default_sys_config, g_default_user_config, TYPE_COUNT, user_callbacks); 

    uint8_t type  = TYPE_INT;
    uint32_t key  = strtoul(argv[0], NULL, 10);
    char * value  = argv[1];
    uint16_t vlen = atoi(argv[2]);

    int64_t i = rfs_set(pfs, 0, type, &key, value, vlen, NULL, 0);
    if (i == -1)
    {
        printf("rfs_set failed: key %u (type: %u, value: %s, vlen: %hu)\n", key, type, value, vlen);
        return -1;
    }

    stIndex index;
    int64_to_index(i, &index);
    printf("key %u (type: %u, value: %s, vlen: %hu) stored at file_type: %hu, file_no: %hu, grid_idx: %u\n", key, type, value, vlen, index.file.file_type, index.file.file_no, index.grid_idx);

    return 0;
}

static int get_int(int argc, char **argv)
{
    if (argc != 1)
    {
        cmd_t *cmd = get_cmd(__FUNCTION__);
        printf("format error. \n%s %s\n", cmd->name, cmd->help);
        return -1;
    }

    rfs * pfs = rfs_create(g_default_sys_config, g_default_user_config, TYPE_COUNT, user_callbacks); 

    uint8_t type  = TYPE_INT;
    uint32_t key  = strtoul(argv[0], NULL, 10);

    char value[1024 * 4];
    uint16_t vlen = 0;
    int64_t i = rfs_get(pfs, type, &key, value, &vlen, NULL, 0);
    value[vlen] = '\0';

    if (i == -1)
    {
        printf("rfs_get failed: key %u (type: %u)\n", key, type);
        return -1;
    }

    stIndex index;
    int64_to_index(i, &index);
    printf("key %u (type: %u, value: %s, vlen: %hu) stored at file_type: %hu, file_no: %hu, grid_idx: %u\n", key, type, value, vlen, index.file.file_type, index.file.file_no, index.grid_idx);

    return 0;
}

static int del_int(int argc, char **argv)
{
    if (argc != 1)
    {
        cmd_t *cmd = get_cmd(__FUNCTION__);
        printf("format error. \n%s %s\n", cmd->name, cmd->help);
        return -1;
    }

    rfs * pfs = rfs_create(g_default_sys_config, g_default_user_config, TYPE_COUNT, user_callbacks); 

    uint8_t type  = TYPE_INT;
    uint32_t key  = strtoul(argv[0], NULL, 10);

    int ret = rfs_del(pfs, type, &key, NULL, 0);

    if (ret == -1)
    {
        printf("rfs_del failed: key %u (type: %u)\n", key, type);
        return -1;
    }

    printf("rfs_del succeed: key %u (type: %u)\n", key, type);

    return 0;
}

static int set_string(int argc, char **argv)
{
    if (argc != 3)
    {
        cmd_t *cmd = get_cmd(__FUNCTION__);
        printf("format error. \n%s %s\n", cmd->name, cmd->help);
        return -1;
    }

    rfs * pfs = rfs_create(g_default_sys_config, g_default_user_config, TYPE_COUNT, user_callbacks); 

    uint8_t type  = TYPE_STRING;
    char * key    = argv[0];
    char * value  = argv[1];
    uint16_t vlen = atoi(argv[2]);

    int64_t i = rfs_set(pfs, 0, type, key, value, vlen, NULL, 0);
    if (i == -1)
    {
        printf("rfs_set failed: key %s (type: %u, value: %s, vlen: %hu)\n", key, type, value, vlen);
        return -1;
    }

    stIndex index;
    int64_to_index(i, &index);
    printf("key %s (type: %u, value: %s, vlen: %hu) stored at file_type: %hu, file_no: %hu, grid_idx: %u\n", key, TYPE_STRING, value, vlen, index.file.file_type, index.file.file_no, index.grid_idx);

    return 0;
}

static int get_string(int argc, char **argv)
{
    if (argc != 1)
    {
        cmd_t *cmd = get_cmd(__FUNCTION__);
        printf("format error. \n%s %s\n", cmd->name, cmd->help);
        return -1;
    }

    rfs * pfs = rfs_create(g_default_sys_config, g_default_user_config, TYPE_COUNT, user_callbacks); 

    uint8_t type  = TYPE_STRING;
    char * key    = argv[0];

    char value[1024 * 4];
    uint16_t vlen = 0;
    int64_t i = rfs_get(pfs, type, key, value, &vlen, NULL, 0);
    value[vlen] = '\0';

    if (i == -1)
    {
        printf("rfs_get failed: key %s (type: %u)\n", key, type);
        return -1;
    }

    stIndex index;
    int64_to_index(i, &index);
    printf("key %s (type: %u, value: %s, vlen: %hu) stored at file_type: %hu, file_no: %hu, grid_idx: %u\n", key, type, value, vlen, index.file.file_type, index.file.file_no, index.grid_idx);

    return 0;
}

static int del_string(int argc, char **argv)
{
    if (argc != 1)
    {
        cmd_t *cmd = get_cmd(__FUNCTION__);
        printf("format error. \n%s %s\n", cmd->name, cmd->help);
        return -1;
    }

    rfs * pfs = rfs_create(g_default_sys_config, g_default_user_config, TYPE_COUNT, user_callbacks); 

    uint8_t type  = TYPE_STRING;
    char * key    = argv[0];

    int ret = rfs_del(pfs, type, key, NULL, 0);
    if (ret == -1)
    {
        printf("rfs_del failed: key %s (type: %u)\n", key, type);
        return -1;
    }

    printf("rfs_del succeed: key %s (type: %u)\n", key, type);

    return 0;
}

static int print_data(int argc, char **argv)
{
    rfs * pfs = rfs_create(g_default_sys_config, g_default_user_config, TYPE_COUNT, user_callbacks); 

    rfs_print_data(pfs);

    return 0;
}

static int print_hashtable(int argc, char **argv)
{
    rfs * pfs = rfs_create(g_default_sys_config, g_default_user_config, TYPE_COUNT, user_callbacks); 

    rfs_print_hashtable(pfs);

    return 0;
}

cmd_t g_cmd_list[] = 
{
#define DEFINE_CMD(cmd, arg) {#cmd, arg, cmd}
    DEFINE_CMD(set_int, "int value vlen"),
    DEFINE_CMD(get_int, "int"),
    DEFINE_CMD(del_int, "int"),
    DEFINE_CMD(set_string, "string value vlen"),
    DEFINE_CMD(get_string, "string"),
    DEFINE_CMD(del_string, "string"),
    DEFINE_CMD(print_data, ""),
    DEFINE_CMD(print_hashtable, ""),
#undef DEFINE_CMD
};

//tool

static cmd_t *get_cmd(const char *name)
{
    int i = 0;
    for (; i < (int)DIM(g_cmd_list); i++) 
    {
        if (strcasecmp(name, g_cmd_list[i].name) == 0) 
            return g_cmd_list+ i;
    }
    return NULL;
}

static void usage(const char *argv0)
{
    unsigned int i;
    printf("Usage:\n");
    for (i = 0; i < DIM(g_cmd_list); i++) 
    {
        printf("%s %s %s\n", argv0, g_cmd_list[i].name, g_cmd_list[i].help);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) 
    { 
        usage(argv[0]); 
        return 1; 
    }

    cmd_t *cmd = get_cmd(argv[1]);
    if (! cmd)
    {
        usage(argv[0]);
        return -1;
    }

    return cmd->handle_func(argc - 2, argv + 2);
}

