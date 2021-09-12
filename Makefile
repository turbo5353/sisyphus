all:
	gcc `pkg-config gtk+-3.0 --cflags` -Wall -o sisyphus sisyphus.c `pkg-config gtk+-3.0 --libs`
