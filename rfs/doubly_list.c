#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "doubly_list.h"

typedef struct {
#if 0
    int group_id;
#endif
    int prec;
    int next;
} stDoublyListNode;

struct _stDoublyList {
    int group_count;
    int *groups;
    int node_count;
    stDoublyListNode * pn;
};

stDoublyList * dl_create(int group_count, int node_count)
{
    assert(node_count > 0);

    stDoublyList * pdl = calloc(1, sizeof(stDoublyList));
    if (pdl == NULL)
        return NULL;

    pdl->group_count = group_count;
    pdl->groups = calloc(group_count, sizeof(int));
    if (pdl->groups == NULL)
    {
        free(pdl);
        return NULL;
    }

    pdl->node_count = node_count;
    pdl->pn = calloc(node_count, sizeof(stDoublyListNode));
    if (pdl->pn == NULL)
    {
        free(pdl);
        free(pdl->groups);
        return NULL;
    }

    return pdl;
}

int dl_init_group(stDoublyList * pdl, int group_id)
{
    assert(pdl != NULL);
    assert(group_id >= 0 && group_id < pdl->group_count);

    int * groups = pdl->groups;
    stDoublyListNode * pn = pdl->pn;

    int gid = 0;
    for (; gid < pdl->group_count ;++gid)
        groups[gid] = -1;

    groups[group_id] = 0;

    int idx = 0;
    for(; idx < pdl->node_count - 1; ++idx)
    {
#if 0
        pn[idx].group_id = group_id;
#endif
        pn[idx].prec = idx - 1;
        pn[idx].next = idx + 1;
    }

    int last = pdl->node_count - 1;
#if 0
    pn[last].group_id = group_id;
#endif
    pn[last].prec = last - 1;
    pn[last].next = -1;

    return 0;
}

int dl_destroy(stDoublyList * pdl)
{
    assert(pdl != NULL);

    free(pdl->groups);
    free(pdl->pn);
    free(pdl);

    return 0;
}

int dl_get_group_head(stDoublyList * pdl, int group_id)
{
    assert(pdl != NULL);
    assert(group_id >= 0 && group_id < pdl->group_count);

    return pdl->groups[group_id];
}

int dl_get_group_next(stDoublyList * pdl, int group_id, int idx)
{
    assert(pdl != NULL);
    assert(group_id >= 0 && group_id < pdl->group_count);

    if((idx < 0) || (idx >= pdl->node_count))
        return -1;

#if 0
    assert(pn[idx].group_id == group_id);
#endif

    return pdl->pn[idx].next;
}

int dl_move_idx(stDoublyList * pdl, int idx, int src_group_id, int dst_group_id)
{
    assert(pdl != NULL);
    assert((idx >= 0) && (idx < pdl->node_count));
    assert(src_group_id >= 0 && src_group_id < pdl->group_count);
    assert(dst_group_id >= 0 && dst_group_id < pdl->group_count);

    int * groups = pdl->groups;
    stDoublyListNode * pn = pdl->pn;

#if 0
    assert(pn[idx].group_id == src_group_id);
#endif

    //detach from old group list
    int prec = pn[idx].prec;
    int next = pn[idx].next;
    if (prec != -1) pn[prec].next = next;
    if (next != -1) pn[next].prec = prec;

    if (groups[src_group_id] == idx)
        groups[src_group_id] = next;

    //attach to new group list
    int new_group_head = groups[dst_group_id];
    if (new_group_head == -1)
    {
        pn[idx].prec  = -1;
        pn[idx].next  = -1;
        groups[dst_group_id] = idx; 
    }
    else
    {
        int prec = pn[new_group_head].prec;
        pn[idx].prec = prec;
        pn[idx].next = new_group_head;
        pn[new_group_head].prec = idx;

        groups[dst_group_id] = idx; 
    }

#if 0
    pn[idx].group_id = dst_group_id;
#endif

    return 0;
}

int dl_print(stDoublyList * pdl)
{
    assert(pdl != NULL);

    printf("group_count: %d\n", pdl->group_count);
    printf("node_count : %d\n", pdl->node_count);

    int * groups = pdl->groups;
    stDoublyListNode * pn = pdl->pn;

    int gid = 0;
    for (; gid < pdl->group_count; ++gid)
    {
        printf("groups[%d]: head = %d\n\tnode \tprec \tnext\n", gid, groups[gid]);
        int idx = groups[gid];
        while (idx != -1)
        {
            printf("\t%d \t%d \t%d\n", idx, pdl->pn[idx].prec, pdl->pn[idx].next);
            idx = pn[idx].next;
        }
    }

    return 0;
}

