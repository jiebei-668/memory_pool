all: demo01 demo02
demo01: demo01.c
	gcc -g -o demo01 demo01.c -std=c99
demo02: demo02.c
	gcc -g -o demo02 demo02.c
clean:
	rm -rf demo01 demo02
