C := gcc
CFLAGS := -Wall -g -DMAX_KEY_LEN=12

incs = -I./gtest/include/ -I../rfs/ -I../demo/
libs = -L./gtest/lib/

target = unit

$(target): unittest.cpp .objs/doubly_list.o .objs/singly_list.o .objs/hash_table.o
	g++ $(CFLAGS) $(incs) $^ -lpthread $(libs) -lgtest -lgtest_main -o $@ 

.objs/doubly_list.o: ../rfs/doubly_list.c
	$(C) $(CFLAGS) -c $< -o $@

.objs/singly_list.o: ../rfs/singly_list.c
	$(C) $(CFLAGS) -c $< -o $@

.objs/hash_table.o: ../rfs/hash_table.c
	$(C) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(target)
	@rm -f .objs/*.o
	@rm -f core


