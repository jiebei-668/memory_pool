all: demo01
demo01: demo01.c
	gcc -g -o demo01 demo01.c -std=c99
clean:
	rm -rf demo01
