all:
	gcc `pkg-config gtk+-3.0 --cflags`  -Wall -o sisyphus sisyphus.c tasks.c `pkg-config gtk+-3.0 --libs`
