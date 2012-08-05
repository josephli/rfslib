#ifndef  DOUBLY_LIST_INC
#define  DOUBLY_LIST_INC

struct _stDoublyList;
typedef struct _stDoublyList stDoublyList;

stDoublyList * dl_create(int group_count, int node_count);
int dl_init_group(stDoublyList * pdl, int group_id);
int dl_destroy(stDoublyList * pdl);
int dl_get_group_head(stDoublyList * pdl, int group_id);
int dl_get_group_next(stDoublyList * pdl, int group_id, int idx);
int dl_move_idx(stDoublyList * pdl, int idx, int src_group_id, int dst_group_id);
int dl_print(stDoublyList * pdl);

#endif

