#
# Makefile for JKThreads, kernel-level threads for mltp
#

CC = gcc
CFLAGS = -Wall -I. -O2 -g -D_REENTRANT -D__SMP__ -D__JKFILEIO

all : libjkt.a samples

libjkt.a: jkcthread.o memalloc.o fileio.o
	ar crv $@ $^
	ranlib $@

samples: force
	make -C samples

jkcthread.o : jkcthread.h memalloc.h fileio.h

memalloc.o : jkcthread.h memalloc.h

fileio.o : jkcthread.h fileio.h

force:

clean :
	-rm -f core
	-rm -f *.o
	-rm -f *.a
	make -C samples clean
