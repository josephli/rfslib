#include <gtest/gtest.h>
#include <stdio.h>

extern "C"
{
    #include "doubly_list.h"
    #include "singly_list.h"
    #include "hash_table.h"
    #include "user.h"
}

TEST(rfslib, doubly_list)
{
    enum {
        grpIdle = 0,
        grpUsed = 1,
        grpCount
    };

    stDoublyList * pdl = dl_create(grpCount, 5);
    dl_init_group(pdl, grpIdle);

    int idle_head = dl_get_group_head(pdl, grpIdle);
    EXPECT_EQ(idle_head, 0);

    int expect_next = 0;
    int next = idle_head;
    for (; next != -1; next = dl_get_group_next(pdl, grpIdle, next))
        EXPECT_EQ(next, expect_next++);

    int used_head = dl_get_group_head(pdl, grpUsed);
    EXPECT_EQ(used_head, -1);
    EXPECT_EQ(dl_get_group_next(pdl, grpUsed, used_head), -1);

    dl_move_idx(pdl, 0, grpIdle, grpUsed);
    EXPECT_EQ(dl_get_group_head(pdl, grpIdle), 1);
    EXPECT_EQ(dl_get_group_head(pdl, grpUsed), 0);

    dl_move_idx(pdl, 0, grpUsed, grpIdle);
    EXPECT_EQ(dl_get_group_head(pdl, grpIdle), 0);
    EXPECT_EQ(dl_get_group_head(pdl, grpUsed), -1);

    dl_move_idx(pdl, 1, grpIdle, grpUsed);
    dl_move_idx(pdl, 2, grpIdle, grpUsed);
    dl_move_idx(pdl, 3, grpIdle, grpUsed);
    EXPECT_EQ(dl_get_group_head(pdl, grpIdle), 0);
    EXPECT_EQ(dl_get_group_head(pdl, grpUsed), 3);

    dl_move_idx(pdl, 3, grpUsed, grpIdle);
    EXPECT_EQ(dl_get_group_head(pdl, grpIdle), 3);
    EXPECT_EQ(dl_get_group_head(pdl, grpUsed), 2);

    dl_move_idx(pdl, 1, grpUsed, grpIdle);
    EXPECT_EQ(dl_get_group_head(pdl, grpIdle), 1);
    EXPECT_EQ(dl_get_group_head(pdl, grpUsed), 2);
}

TEST(rfslib, singly_list)
{
    stSinglyList * psl = sl_create(5);

    EXPECT_EQ(sl_peek_idle_idx(psl), 0);

    sl_consume_idle_idx(psl, 0);
    EXPECT_EQ(sl_peek_idle_idx(psl), 1);

    sl_recycle_used_idx(psl, 0);
    EXPECT_EQ(sl_peek_idle_idx(psl), 0);

    sl_consume_idle_idx(psl, 0);
    EXPECT_EQ(sl_peek_idle_idx(psl), 1);
    sl_consume_idle_idx(psl, 1);
    EXPECT_EQ(sl_peek_idle_idx(psl), 2);
    sl_consume_idle_idx(psl, 2);
    EXPECT_EQ(sl_peek_idle_idx(psl), 3);
    sl_consume_idle_idx(psl, 3);
    EXPECT_EQ(sl_peek_idle_idx(psl), 4);
    sl_consume_idle_idx(psl, 4);
    EXPECT_EQ(sl_peek_idle_idx(psl), -1);

    sl_recycle_used_idx(psl, 0);
    EXPECT_EQ(sl_peek_idle_idx(psl), 0);
    sl_recycle_used_idx(psl, 4);
    EXPECT_EQ(sl_peek_idle_idx(psl), 4);
}

TEST(rfslib, hash_table)
{
    stKeyCallback user_callbacks[TYPE_COUNT] = {
        {NULL, NULL, NULL, NULL, NULL, NULL},
        {int_hash, int_type, int_print, int_cmp, int_serialize, int_deserialize},
        {string_hash, string_type, string_print, string_cmp, string_serialize, string_deserialize}
    };

    stHashTable * pht = hashtable_create(2, 4);

    {
        int ikey = 10717972;
        stIndex index = {{2, 256}, 1024};
        hashtable_set(pht, &ikey, &index, user_callbacks + 1);
        hashtable_get(pht, &ikey, &index, NULL, user_callbacks + 1);
        EXPECT_EQ(index.file.file_type, (uint16_t) 2);
        EXPECT_EQ(index.file.file_no, (uint16_t) 256);
        EXPECT_EQ(index.grid_idx, (uint16_t) 1024);

        stIndex new_index = {{4, 512}, 2048};
        hashtable_set(pht, &ikey, &new_index, user_callbacks + 1);
        hashtable_get(pht, &ikey, &new_index, NULL, user_callbacks + 1);
        EXPECT_EQ(new_index.file.file_type, (uint16_t) 4);
        EXPECT_EQ(new_index.file.file_no, (uint16_t) 512);
        EXPECT_EQ(new_index.grid_idx, (uint16_t) 2048);

        hashtable_del(pht, &ikey, user_callbacks + 1);
        EXPECT_EQ(hashtable_get(pht, &ikey, &new_index, NULL, user_callbacks + 1), -1);
    }

    {
        char skey[] = "ripwu";
        stIndex index = {{2, 256}, 1024};
        hashtable_set(pht, skey, &index, user_callbacks + 2);
        hashtable_get(pht, skey, &index, NULL, user_callbacks + 2);
        EXPECT_EQ(index.file.file_type, (uint16_t) 2);
        EXPECT_EQ(index.file.file_no, (uint16_t) 256);
        EXPECT_EQ(index.grid_idx, (uint16_t) 1024);

        stIndex new_index = {{4, 512}, 2048};
        hashtable_set(pht, skey, &new_index, user_callbacks + 2);
        hashtable_get(pht, skey, &new_index, NULL, user_callbacks + 2);
        EXPECT_EQ(new_index.file.file_type, (uint16_t) 4);
        EXPECT_EQ(new_index.file.file_no, (uint16_t) 512);
        EXPECT_EQ(new_index.grid_idx, (uint16_t) 2048);

        hashtable_del(pht, skey, user_callbacks + 2);
        EXPECT_EQ(hashtable_get(pht, skey, &new_index, NULL, user_callbacks + 2), -1);
    }
}

