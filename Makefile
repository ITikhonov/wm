CFLAGS=-Wall -g -Werror
LDLIBS=-lxcb -lxcb-icccm -lxcb-keysyms -lxcb-util

all: wm wm2 wmlist if wmclose wmkeys

wm: LDLIBS=-lX11
wm: wm.o dumpevent.o

wm2: wm2.o

#hello: LDLIBS=-lX11
#hello: hello.c

