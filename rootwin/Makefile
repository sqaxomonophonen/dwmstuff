all: rootwin

rootwin: rootwin.c
	$(CC) -g -O0 -lm -lX11 rootwin.c -o rootwin

install:
	install rootwin /usr/local/bin/rootwin

clean:
	rm -f rootwin
