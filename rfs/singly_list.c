#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "singly_list.h"

typedef struct 
{
    int next;
} stSinglyListNode;

struct _stSinglyList
{
	int node_count;
	int head;
	stSinglyListNode * pn;
};

static int _sl_init(stSinglyList * psl)
{
    assert(psl != NULL);
    psl->head = 0;

    int idx = 0;
    for(; idx < psl->node_count - 1; ++idx)
        psl->pn[idx].next = idx + 1;

    psl->pn[psl->node_count - 1].next = -1;

    return 0;
}

stSinglyList * sl_create(int node_count)
{
    stSinglyList * psl = calloc(1, sizeof(stSinglyList));
    if (psl == NULL)
        return NULL;

    psl->node_count = node_count;
    psl->pn = calloc(node_count, sizeof(stSinglyListNode));
    if (psl->pn == NULL)
    {
        free(psl);
        return NULL;
    }

    _sl_init(psl);

    return psl;
}

int sl_destroy(stSinglyList * psl)
{
    assert(psl != NULL);

    free(psl->pn);
    free(psl);

    return 0;
}

int sl_peek_idle_idx(stSinglyList * psl)
{
    assert(psl != NULL);

    return psl->head;
}

int sl_consume_idle_idx(stSinglyList * psl, int idx)
{
    assert(psl != NULL);

    if ((idx < 0) || (idx >= psl->node_count))
        return -1;

    assert(psl->head == idx);

	stSinglyListNode * pn = psl->pn;

    int next     = pn[idx].next;
    pn[idx].next = -1;
    psl->head    = next;

    return 0;
}

int sl_recycle_used_idx(stSinglyList * psl, int idx)
{
    assert(psl != NULL);

    if ((idx < 0) || (idx >= psl->node_count))
        return -1;

	stSinglyListNode * pn = psl->pn;
    assert(pn[idx].next == -1);

	pn[idx].next = psl->head;
	psl->head    = idx;

	return 0;
}

int sl_print(stSinglyList * psl)
{
    assert(psl != NULL);

    printf("node_count: %d\n", psl->node_count);
    printf("head: %d\n", psl->head);

    int idx = 0;
    for(; idx < psl->node_count; ++idx)
        printf("node[%d].next: %d\n", idx, psl->pn[idx].next);

    return 0;
}

