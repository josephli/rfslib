#ifndef  CONFIG_INC
#define  CONFIG_INC

#include <stdint.h>

//1.背景:
//        rfs_set时如果新数据不保存于原来的格子,
//        为了防止数据丢失,会先将新数据写入新文件,然后将旧数据从旧文件删除.
//2.问题:
//        新数据已写入,此时出现异常(如库存在bug或者机器断电),导致旧数据删除失败
//        程序重启后执行rfs_start函数时可以发现这种异常(新旧数据的key一致,加载到hash_table时会冲突)
//3.处理方法:
//        此时rfs库应该执行的操作为:
enum {
    DELETE_OLD_DATA = 1, //删除旧数据
    IGNORE_OLD_DATA = 2, //忽略旧数据
    DELETE_NEW_DATA = 3, //删除新数据
    IGNORE_NEW_DATA = 4, //忽略新数据
};

//stUserConfig: 在程序启动前可以根据需要修改参数
typedef struct {
    uint8_t auto_repair;           //rfs库中存在两个一样的key时,rfs库执行的操作
    uint8_t size_down_if_possible; //是否尽量使用更小类型的文件来存储数据
                                   //前提: rfs_set时新数据比旧数据长度更小,
                                   //      且有更小类型的文件可以存储下新数据
    uint8_t check_key_when_get;    //rfs_get时比较key和根据index_map得到的文件中的key是否一致
    uint8_t check_key_when_set;    //rfs_set时比较key和根据index_map得到的文件中的key是否一致
} stUserConfig;

extern stUserConfig g_default_user_config;

typedef struct {
    char working_dir[256];       //rfs工作目录
    char file_name_format[256];  //文件名的格式
    uint16_t max_file_type_num;  //最多有多少种文件类型
    uint16_t max_open_file_num;  //每种类型最多允许打开多少文件
    uint32_t file_size;          //单个文件有效数据的大小
    uint32_t base_file_grid_size;//最小类型的文件一个格子占多少字节
    uint16_t grid_size_growth_factor; //第N种类型的文件格子大小是第N-1种的多少倍
    uint32_t hashtable_list_num; //hashtable中桶的个数
    uint32_t hashtable_node_num; //hashtable允许存储的最大item个数
} stSysConfig;

extern stSysConfig g_default_sys_config;

#endif

