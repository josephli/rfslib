C := gcc
CFLAGS := -Wall -g -DMAX_KEY_LEN=16

obj_dir = .objs/

incs = -I../rfs/
libs = ../rfs/rfslib.a

heads = $(wildcard *.h)
srcs = $(wildcard *.c)
pure_objs = $(patsubst %.c,%.o,$(srcs))
objs = $(addprefix $(obj_dir), $(pure_objs))

target = demo

all:$(target)

$(target): $(objs) $(heads) $(libs)
	$(C) $(CFLAGS) $(incs) -o $@ $^ 

$(obj_dir)%.o: %.c
	$(C) $(CFLAGS) $(incs) -c $< -o $@

clean:
	@rm -f $(obj_dir)*.o
	@rm -f $(target)
	@rm -f core

