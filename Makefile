CFLAGS=-Wall -g
LDLIBS=-lX11

all: wm

wm: wm.o dumpevent.o
