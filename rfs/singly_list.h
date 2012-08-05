#ifndef  SINGLY_LIST_INC
#define  SINGLY_LIST_INC

struct _stSinglyList;
typedef struct _stSinglyList stSinglyList;

stSinglyList * sl_create(int node_count);
int sl_destroy(stSinglyList * psl);
int sl_peek_idle_idx(stSinglyList * psl);
int sl_consume_idle_idx(stSinglyList * psl, int idx);
int sl_recycle_used_idx(stSinglyList * psl, int idx);
int sl_print(stSinglyList * psl);

#endif

