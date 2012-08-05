C := gcc
CFLAGS := -Wall -g -DMAX_KEY_LEN=16
AR := ar
ARFLAGS := rcs

obj_dir = .objs/

heads = $(wildcard *.h)
srcs = $(wildcard *.c)
pure_objs = $(patsubst %.c,%.o,$(srcs))
objs = $(addprefix $(obj_dir), $(pure_objs))

target = rfslib.a

all:$(target)

$(target): $(objs) $(heads)
	$(AR) $(ARFLAGS) $@ $^

$(obj_dir)%.o: %.c
	$(C) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(obj_dir)*.o
	@rm -f $(target)
	@rm -f core

