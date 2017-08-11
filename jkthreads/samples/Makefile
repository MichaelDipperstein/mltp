#
# Makefile for JKThreads sample code.  Requires existing build of JKThreads
#

CFLAGS = -Wall -I.. -O2 -g -D_REENTRANT -D__SMP__
LIBS = -L.. -ljkt
CC = gcc

all : jktest jkpi jdyn_mm jfix_mm

jktest : jktest.o ../libjkt.a
	$(CC) $(CFLAGS) $@.o $(LIBS) -o $@

jkpi : jkpi.o ../libjkt.a
	$(CC) $(CFLAGS) $@.o $(LIBS) -o $@

jdyn_mm : jdyn_mm.o ../libjkt.a
	$(CC) $(CFLAGS) $@.o $(LIBS) -o $@

jfix_mm : jfix_mm.o ../libjkt.a
	$(CC) $(CFLAGS) $@.o $(LIBS) -o $@

jktest.o : ../jkcthread.h

jkpi.o : ../jkcthread.h

jdyn_mm.o : ../jkcthread.h

jfix_mm.o : ../jkcthread.h

clean :
	-rm -f core
	-rm -f *.o
