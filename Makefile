CFLAGS=-Wall
LDLIBS=-lX11

all: wm

wm: wm.o dumpevent.o
