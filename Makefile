CFLAGS=-Wall -g
LDLIBS=-lxcb -lxcb-icccm

all: wm wm2 hello

wm: wm.o dumpevent.o
wm2: wm2.o

hello: LDLIBS=-lX11
hello: hello.c

