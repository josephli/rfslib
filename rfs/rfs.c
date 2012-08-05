#include "rfs.h"
#include "doubly_list.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/times.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CHK_RET(x) do { if (x != 0) return -1; } while (0)

typedef struct {
    FILE *         fp;
    char           path[256];
    stDoublyList * grids_translist;
} stFileInfo;

enum {
    grpIdle = 0,
    grpUsed = 1,
    grpCount
};

typedef struct {
    uint16_t file_type;
    uint16_t file_no;
    uint32_t grid_num;
    uint32_t grid_size;
} _stFileHeader;

typedef struct {
    union {
        _stFileHeader header;
        char  buf[1024];
    };
} stFileHeader;

typedef struct {
    uint32_t write_time;
} _stGridHeader;

typedef struct {
    union {
        _stGridHeader header;
        char buf[32];
    };
} stGridHeader;

/*
   +------+ 
   | file |             0            1            ...       gird_num-1 
   +-------------------------------------------------------------------+
   | stFileHeader |    grid    |    grid    |    ......   |    gird    |
   +-------------------------------------------------------------------+
   -> gird_size <-     

   +------+
   | grid |                          
   +-------------------------------------------------+      
   | stGridHeader | type | klen | key | vlen | value |
   +-------------------------------------------------+      
*/

typedef struct {
    uint16_t max_opened_file_no;
    uint32_t grid_num;
    uint32_t grid_size;
    stFileInfo * file_info_array;
} stFileTypeMng;

struct _rfs 
{
    stSysConfig     sys_config;
    stUserConfig    user_config;
    uint8_t         type_count;
    stKeyCallback * user_callbacks;
    stHashTable   * hash_table;
    stFileTypeMng * type_mng_array;

    char          * private_data;
};

static int _load_file(rfs * pfs, char * file)
{
    FILE * fp = fopen(file, "r+");
    if (fp == NULL)
    {
        printf("(%s:%s)\tfailed to open file %s, reason: %s\n",
                __FILE__, __FUNCTION__, file, strerror(errno));
        return -1;
    }

    stFileHeader file_header;
    fread(&file_header, sizeof(stFileHeader), 1, fp);

    uint16_t file_type = file_header.header.file_type;
    uint16_t file_no   = file_header.header.file_no;
    uint32_t grid_num  = file_header.header.grid_num;
    uint32_t grid_size = file_header.header.grid_size;

    stSysConfig * psc = &pfs->sys_config;
    if (file_type >= psc->max_file_type_num || file_no >= psc->max_open_file_num)
    {
        //TODO log error
        return -1;
    }

    stFileTypeMng * pftm = pfs->type_mng_array + file_type;

    if (grid_num != pftm->grid_num || grid_size != pftm->grid_size)
    {
        //TODO log error
        return -1;
    }

    if (file_no > pftm->max_opened_file_no)
        pftm->max_opened_file_no = file_no;

    stFileInfo * pfi = pftm->file_info_array + file_no;

    pfi->fp = fp;
    strncpy(pfi->path, file, strlen(file));
    if (pfi->grids_translist == NULL)
    {
        pfi->grids_translist = dl_create(grpCount, pftm->grid_num);
        dl_init_group(pfi->grids_translist, grpIdle);
    }

    stIndex index;
    index.file.file_type = file_type;
    index.file.file_no   = file_no;

    fseek(fp, 0, SEEK_END);
    int64_t length = ftell(fp);
    if (length == -1)
    {
        printf("(%s:%s)\tfailed to ftell file %s, reason: %s\n",
                __FILE__, __FUNCTION__, file, strerror(errno));
        return -1;
    }

    fseek(fp, sizeof(stFileHeader), SEEK_SET);

    char key[MAX_KEY_LEN+1];
    uint32_t idx = 0;
    for (; idx < grid_num; ++idx)
    {
        uint32_t read_size = MIN((length - sizeof(stFileHeader) - grid_size * idx), grid_size);
        char * p = pfs->private_data;
        fread(p, 1, read_size, fp);

        p += sizeof(stGridHeader);
        uint8_t type = *(uint8_t *) p;
        if (type == 0 || type >= pfs->type_count)
            continue;
        p += sizeof(uint8_t);

        stKeyCallback * cb = pfs->user_callbacks + type;

        uint16_t len  = *(uint16_t *) p;
        if (len == 0)
            continue;
        p += sizeof(uint16_t);

        if (cb->deserialize(key, p, len) != 0)
        {
            //TODO log error
            continue;
        }

        if (len > MAX_KEY_LEN)
            continue;

        key[len] = '\0';
        p += len;

#if 0
        char out[MAX_KEY_LEN+1] = {0};
        cb->print(key, out);
        printf("key is %s\n", out);
#endif

        index.grid_idx = idx;
        if (hashtable_set(pfs->hash_table, key, &index, cb) != 0)
        {
            //TODO log error
            continue;
        }

        dl_move_idx(pfi->grids_translist, idx, grpIdle, grpUsed);

        if (read_size != grid_size)
            break;
    }

    return 0;
}

static int _rfs_init(rfs * pfs)
{
    char * working_dir = pfs->sys_config.working_dir;
    char   file[256] = {0};

    if (access(working_dir, 0) != 0)
    {
        printf("(%s:%s)\trfs working directory %s does not exist, check your config!!!\n",
                __FILE__, __FUNCTION__, working_dir);
        return -1;
    }

    DIR *dir = opendir(working_dir);
    if (dir == NULL)
    {
        printf("(%s:%s)\tfailed to open dir %s, reason: %s\n",
                __FILE__, __FUNCTION__, working_dir, strerror(errno));
        return -1;
    }

    struct stat st;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        sprintf(file, "%s/%s", working_dir, ent->d_name);

        stat(file, &st);
        if (S_ISREG(st.st_mode))
        {
            printf("(%s:%s)\tloading file %s\n", __FILE__, __FUNCTION__, file);
            if (_load_file(pfs, file) != 0)
            {
                //TODO log error
                printf("(%s:%s)\tfailed to load file %s\n", __FILE__, __FUNCTION__, file);
                continue;
            }
            printf("(%s:%s)\tsuccessfully load file %s\n", __FILE__, __FUNCTION__, file);
        }
    };

    return 0;
}

rfs * rfs_create(stSysConfig sys_config, stUserConfig user_config, uint8_t type_count, stKeyCallback* user_callbacks)
{
    rfs * pfs             = calloc(1, sizeof(rfs));
    if (pfs == NULL)
        return NULL;

    pfs->sys_config       = sys_config;
    pfs->user_config      = user_config;
    pfs->type_count       = type_count;
    pfs->user_callbacks   = calloc(type_count, sizeof(stKeyCallback));
    if (pfs->user_callbacks == NULL)
        return NULL;

    uint8_t i = 0;
    for (; i < type_count; ++i)
        pfs->user_callbacks[i] = user_callbacks[i];

    pfs->hash_table = hashtable_create(sys_config.hashtable_list_num, sys_config.hashtable_node_num);
    if (pfs->hash_table == NULL)
        return NULL;

    pfs->type_mng_array = calloc(sys_config.max_file_type_num, sizeof(stFileTypeMng));
    if (pfs->type_mng_array == NULL)
        return NULL;

    uint16_t file_type = 0;
    stFileTypeMng * pftm;
    for (; file_type < sys_config.max_file_type_num; ++file_type)
    {
        pftm = pfs->type_mng_array + file_type;

        pftm->max_opened_file_no = 0;
        if (file_type == 0)
            pftm->grid_size = sys_config.base_file_grid_size;
        else
        {
            stFileTypeMng * p_pre_mng = pfs->type_mng_array + file_type - 1;
            pftm->grid_size = p_pre_mng->grid_size * sys_config.grid_size_growth_factor;
        }
        pftm->grid_num = sys_config.file_size / pftm->grid_size;
        pftm->file_info_array = calloc(sys_config.max_open_file_num, sizeof(stFileInfo));
    }

    pfs->private_data = calloc(1, pftm->grid_size);

    _rfs_init(pfs);

    return pfs;
}

int rfs_destroy(rfs * pfs)
{
    assert(pfs != NULL);

    free(pfs->user_callbacks);
    hashtable_destroy(pfs->hash_table);

    uint16_t type = 0;
    for (; type < pfs->sys_config.max_file_type_num; ++type)
    {
        stFileTypeMng * pftm = pfs->type_mng_array + type;
        if (pftm == NULL)
            continue;

        uint16_t no = 0;
        for (; no <= pftm->max_opened_file_no; ++no)
        {
            stFileInfo * pfi = pftm->file_info_array + no;
            if (pfi == NULL)
                continue;

            if (pfi->fp != NULL)
                fclose(pfi->fp);

            if (pfi->grids_translist != NULL)
                dl_destroy(pfi->grids_translist);

            free(pfi);
        }
        free(pftm);
    }
    free(pfs->type_mng_array);

    free(pfs->private_data);
    free(pfs);

    return 0;
}

//找到grid_size >= size的最小文件类型
static int _get_file_type(rfs * pfs, uint16_t start_type, uint32_t size)
{
    assert(start_type < pfs->sys_config.max_file_type_num);

    uint16_t file_type = start_type;
    for (; file_type < pfs->sys_config.max_file_type_num; ++file_type)
    {
        stFileTypeMng * pftm = pfs->type_mng_array + file_type;
        if (pftm->grid_size >= size)
            return file_type;
    }

    return -1;
}

static FILE * _create_file(rfs * pfs, uint16_t file_type, uint16_t file_no, uint32_t grid_num, uint32_t grid_size)
{
    stSysConfig * psc = &pfs->sys_config;

    char name[256];
    sprintf(name, "%s/%s", psc->working_dir, psc->file_name_format);

    //template matching and replacing, fuck clearsilver, brute force is enough, 
#define TMR(ret, dst, template, matcher, replacer, replace) do { \
    char working[256] = {0}; \
    char *pos = strstr(template, matcher); \
    if (pos == NULL) { ret = -1; break; } \
    strncpy(working, template, pos-template); \
    strcat (working, replacer); \
    strcat (working, pos+strlen(matcher)); \
    sprintf(dst, working, replace); \
    ret = 0; } while (0)
#define RET_NULL(x) do { if (x != 0) return NULL; } while(0)

    int ret = -1;
    TMR(ret, name, name, "$(file_type)", "%d", file_type);
    RET_NULL(ret);

    TMR(ret, name, name, "$(file_no)",   "%d", file_no);
    RET_NULL(ret);

    TMR(ret, name, name, "$(grid_num)",  "%d", grid_num);
    RET_NULL(ret);

    TMR(ret, name, name, "$(grid_size)", "%d", grid_size);
    RET_NULL(ret);

#undef RET_NULL
#undef TMR

    FILE * fp = fopen(name, "w+");
    if (fp == NULL)
    {
        printf("(%s:%s)\tfailed to open file %s, reason: %s\n", 
                __FILE__, __FUNCTION__, name, strerror(errno));
        return NULL;
    }

    stFileHeader header;
    header.header.file_type = file_type;
    header.header.file_no   = file_no;
    header.header.grid_num  = pfs->type_mng_array[file_type].grid_num;
    header.header.grid_size = pfs->type_mng_array[file_type].grid_size;

    fwrite(&header, sizeof(header), 1, fp);
    truncate(name, grid_size * grid_num + sizeof(stFileHeader));

    return fp;
}

//在[begin_type, end_type]中找到grid_size >= size的最小文件类型, 文件号和格子下标
static int _get_idx(rfs * pfs, uint16_t begin_type, uint16_t end_type, uint32_t size, uint16_t * file_type, uint16_t * file_no, uint32_t * grid_idx)
{
    stSysConfig * psc = &pfs->sys_config;

    int ftype = begin_type;
    uint16_t fno   = 0;
    while (1)
    {
        int new_ftype = _get_file_type(pfs, ftype, size);
        if (new_ftype != begin_type && new_ftype == ftype)
            ftype = new_ftype + 1;
        else ftype = new_ftype;

        if (ftype == -1 || ftype > end_type)
        {
            //TODO log error, alert
            return -1;
        }

        stFileTypeMng * pftm = pfs->type_mng_array + ftype;

        fno = 0;
        for (; fno < psc->max_open_file_num; ++fno)
        {
            stFileInfo * pfi = pftm->file_info_array + fno;
            assert(pfi != NULL);

            if (pfi->fp == NULL)
            {
                assert(pfi->grids_translist == NULL);

                //TODO log error
                pfi->fp = _create_file(pfs, ftype, fno, pftm->grid_num, pftm->grid_size);
                if (pfi->fp == NULL)
                    return -1;

                pfi->grids_translist = dl_create(grpCount, pftm->grid_num);
                dl_init_group(pfi->grids_translist, grpIdle);
                if (fno > pftm->max_opened_file_no)
                    pftm->max_opened_file_no = fno;
            }

            int idx = dl_get_group_head(pfi->grids_translist, grpIdle);
            if (idx < 0)
                continue;

            *file_type = ftype;
            *file_no   = fno;
            *grid_idx  = (uint32_t) idx;

            return 0;
        }
    }

    return -1;
}

static int _write(rfs * pfs, FILE * fp, uint16_t real_len, stGridHeader * grid_header, uint8_t type, char * key, uint16_t klen, char * value, uint16_t vlen)
{
    assert(fp != NULL);

    //TODO optimize, reduce system call num

    char * p = pfs->private_data;

    memcpy(p, grid_header, sizeof(stGridHeader));
    p += sizeof(stGridHeader);

    memcpy(p, &type, sizeof(uint8_t));
    p += sizeof(uint8_t);

    memcpy(p, &klen, sizeof(uint16_t));
    p += sizeof(uint16_t);

    memcpy(p, key, klen);
    p += klen;

    memcpy(p, &vlen, sizeof(uint16_t));
    p += sizeof(uint16_t);

    memcpy(p, value, vlen);
    p += vlen;

    assert(p-pfs->private_data == real_len);

    uint16_t r = fwrite(pfs->private_data, 1, real_len, fp);
    if (r == real_len) return 0;
    else return -1;

    return 0;
}

static int _set_grid(rfs * pfs, FILE * fp, uint32_t offset, uint32_t now, uint16_t real_len, uint8_t type, char * key, uint16_t klen, char * value, uint16_t vlen)
{
    fseek(fp, offset, SEEK_SET);

    stGridHeader grid_header;
    if (now != 0)
        grid_header.header.write_time = now;
    else
        grid_header.header.write_time = time(0);

    return _write(pfs, fp, real_len, &grid_header, type, key, klen, value, vlen);
}

static int _del_grid(rfs * pfs, FILE * fp, uint32_t offset)
{
    fseek(fp, offset + sizeof(stGridHeader), SEEK_SET);

    uint32_t empty = 0;
    uint32_t r = fwrite(&empty, 1, sizeof(uint32_t), fp);
    if (r == sizeof(uint32_t)) return 0;
    else return -1;

    return 0;
}

int64_t rfs_set(rfs * pfs, uint32_t now, uint8_t type, void * key, char * value, uint16_t vlen, char * info, uint16_t ilen)
{
    stSysConfig  * psc = &pfs->sys_config;
    stUserConfig * puc = &pfs->user_config;

    char kbuf[MAX_KEY_LEN] = {0};
    uint16_t klen = MAX_KEY_LEN;

    stKeyCallback * cb = pfs->user_callbacks + type;
    if (cb->serialize(key, kbuf, &klen) != 0)
        return -1;

    //参考文件格式图
    uint16_t real_len = sizeof(stGridHeader) + sizeof(uint8_t) + sizeof(uint16_t) + klen + sizeof(uint16_t) + vlen;

    stIndex index;
    int exist = hashtable_get(pfs->hash_table, key, &index, NULL, cb);
    if (exist == -1)
    {
        uint16_t * ftype = &index.file.file_type;
        uint16_t * fno   = &index.file.file_no;
        uint32_t * gidx  = &index.grid_idx;

        *ftype = 0;
        *fno   = 0;
        *gidx  = 0;

        int ret = _get_idx(pfs, 0, psc->max_file_type_num-1, real_len, ftype, fno, gidx);
        if (ret != 0)
        {
            //TODO log error, alert
            return -1;
        }

        stFileTypeMng * pftm = pfs->type_mng_array  + *ftype;
        stFileInfo    * pfi = pftm->file_info_array + *fno;
        FILE * fp = pfi->fp;

        uint32_t offset = sizeof(stFileHeader) + pftm->grid_size * (*gidx);
        CHK_RET(_set_grid(pfs, fp, offset, now, real_len, type, kbuf, klen, value, vlen));
        CHK_RET(dl_move_idx(pfi->grids_translist, *gidx, grpIdle, grpUsed));
        CHK_RET(hashtable_set(pfs->hash_table, key, &index, cb));

        return index_to_int64(&index);
    }
    else
    {
        uint16_t * ftype = &index.file.file_type;
        uint16_t * fno   = &index.file.file_no;
        uint32_t * gidx  = &index.grid_idx;

        stFileTypeMng * pftm = pfs->type_mng_array + *ftype;
        stFileInfo    * pfi  = pftm->file_info_array + *fno;
        FILE * fp = pfi->fp;
        uint32_t offset = sizeof(stFileHeader) + pftm->grid_size * (*gidx);

        if ((real_len <= pftm->grid_size) && (puc->size_down_if_possible == 0))
        {
            _set_grid(pfs, fp, offset, now, real_len, type, kbuf, klen, value, vlen);
            return index_to_int64(&index);
        }

        uint16_t begin_type;
        uint16_t end_type;
        if (real_len > pftm->grid_size)
        {
            begin_type = *ftype + 1;
            end_type   = psc->max_file_type_num - 1;

            if (begin_type >= psc->max_file_type_num)
            {
                //TODO lor error, alert
                return 0;
            }
        }
        else if ((real_len < pftm->grid_size) && (puc->size_down_if_possible))
        {
            begin_type = 0;
            end_type   = *ftype; // = *ftype - 1;
        }

        stIndex new_index;
        uint16_t * new_ftype = &new_index.file.file_type;
        uint16_t * new_fno   = &new_index.file.file_no;
        uint32_t * new_gidx  = &new_index.grid_idx;

        int ret = _get_idx(pfs, begin_type, end_type, real_len, new_ftype, new_fno, new_gidx);
        if (ret != 0)
        {
            //TODO log error, alert
            return -1;
        }

        //如果file_type和file_no都一样的话,说明是原来的文件,将数据写到原来的grid_idx.
        if ((*new_ftype == *ftype) && (*new_fno == *fno))
        {
            _set_grid(pfs, fp, offset, now, real_len, type, kbuf, klen, value, vlen);
            return index_to_int64(&index);
        }

        //否则,写到新的文件
        stFileTypeMng * new_pftm = pfs->type_mng_array  + *new_ftype;
        stFileInfo    * new_pfi  = new_pftm->file_info_array + *new_fno;
        FILE * new_fp = new_pfi->fp;

        uint32_t new_offset = sizeof(stFileHeader) + new_pftm->grid_size * (*new_gidx);
        CHK_RET(_set_grid(pfs, new_fp, new_offset, now, real_len, type, kbuf, klen, value, vlen));
        CHK_RET(_del_grid(pfs, fp, offset));
        CHK_RET(dl_move_idx(pfi->grids_translist, *gidx, grpUsed, grpIdle));
        CHK_RET(dl_move_idx(new_pfi->grids_translist, *new_gidx, grpIdle, grpUsed));
        CHK_RET(hashtable_set(pfs->hash_table, key, &new_index, cb));

        return index_to_int64(&new_index);
    }

    return -1;
}

int64_t rfs_get(rfs * pfs, uint8_t type, void * key, char * value, uint16_t * vlen, char * info, uint16_t ilen)
{
    stKeyCallback * cb = pfs->user_callbacks + type;

    stIndex index;
    int exist = hashtable_get(pfs->hash_table, key, &index, NULL, cb);
    if (exist == -1)
        return -1;

    uint16_t file_type = index.file.file_type;
    uint16_t file_no   = index.file.file_no;
    uint32_t grid_idx  = index.grid_idx;

    stFileTypeMng * pftm = pfs->type_mng_array   + file_type;
    stFileInfo    * pfi  = pftm->file_info_array + file_no;
    FILE          * fp   = pfi->fp;

    uint32_t offset = sizeof(stFileHeader) + pftm->grid_size * grid_idx + sizeof(stGridHeader) + sizeof(uint8_t);
    fseek(fp, offset, SEEK_SET);

    char * p = pfs->private_data;
    fread(p, 1, pftm->grid_size - sizeof(stGridHeader), fp);

    uint16_t klen = *(uint16_t *) p;
    *vlen = *(uint16_t *) (p + sizeof(uint16_t) + klen);
    strncpy(value, p + sizeof(uint16_t) + klen + sizeof(uint16_t), *vlen);

    return index_to_int64(&index);
}

int rfs_del(rfs * pfs, uint8_t type, void * key, char * info, uint16_t ilen)
{
    stKeyCallback * cb = pfs->user_callbacks + type;

    stIndex index;
    int exist = hashtable_get(pfs->hash_table, key, &index, NULL, cb);
    if (exist == -1)
        return -1;

    uint16_t file_type = index.file.file_type;
    uint16_t file_no   = index.file.file_no;
    uint32_t grid_idx  = index.grid_idx;

    stFileTypeMng * pftm = pfs->type_mng_array   + file_type;
    stFileInfo    * pfi  = pftm->file_info_array + file_no;
    FILE          * fp   = pfi->fp;

    uint32_t offset = sizeof(stFileHeader) + pftm->grid_size * grid_idx;
    _del_grid(pfs, fp, offset);

    dl_move_idx(pfi->grids_translist, grid_idx, grpUsed, grpIdle);

    return hashtable_del(pfs->hash_table, key, cb);
}

int rfs_print_data(rfs * pfs)
{
    char * p = pfs->private_data;
    uint8_t type  = 0;
    int32_t idx   = -1;
    uint16_t klen = 0;

    while (hashtable_next(pfs->hash_table, &idx, &type, p, &klen, pfs->user_callbacks) == 0)
    {
        if (type == 0 || type >= pfs->type_count)
            continue;

        stKeyCallback * cb = pfs->user_callbacks + type;

        char out[MAX_KEY_LEN+1] = {0};
        cb->print(p, out);

        char value[1024 * 4];
        uint16_t vlen = 0;
        int64_t i = rfs_get(pfs, type, p, value, &vlen, NULL, 0);
        value[vlen] = '\0';

        if (i == -1)
        {
            printf("rfs_get failed: key %s\n", out);
            continue;
        }

        stIndex index;
        int64_to_index(i, &index);
        printf("key %s (value: %s, vlen: %hu) stored at file_type: %hu, file_no: %hu, grid_idx: %u\n", 
                out, value, vlen, index.file.file_type, index.file.file_no, index.grid_idx);
    }

    return 0;
}

int rfs_print_hashtable(rfs * pfs)
{
    return hashtable_print(pfs->hash_table, pfs->user_callbacks);
}
