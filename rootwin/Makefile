all: rootwin

rootwin: rootwin.c
	$(CC) -g -O0 -Wall rootwin.c -o rootwin -lX11 -lm

install:
	install rootwin /usr/local/bin/rootwin

clean:
	rm -f rootwin
