CFLAGS=-Wall -g -Werror
LDLIBS=-lxcb -lxcb-icccm -lxcb-aux -lxcb-keysyms

all: wm wm2 hello wmlist if

wm: LDLIBS=-lX11
wm: wm.o dumpevent.o

wm2: wm2.o

hello: LDLIBS=-lX11
hello: hello.c

