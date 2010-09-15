CFLAGS=-Wall -g
LDLIBS=-lxcb -lxcb-icccm

all: wm wm2

wm: wm.o dumpevent.o
wm2: wm2.o

