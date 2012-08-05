dirs = rfs demo unittest

all:
	@for dir in $(dirs); do make -C $$dir; echo; done

clean:
	@for dir in $(dirs); do make clean -C $$dir; echo; done

