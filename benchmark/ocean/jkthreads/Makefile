TARGET = OCEAN
MLTP_DIR = /home/research_home/mdipper/mltp
JK_DIR = $(MLTP_DIR)/jkthreads
OBJS = main.o tlib.o jacobcalc.o jacobcalc2.o laplacalc.o \
	slave1.o slave2.o multi.o linkup.o subblock.o getopt.o
CFLAGS = -O2 -g -w -DTHREAD_BUILD -D_REENTRANT -D__SMP__ -I$(JK_DIR)
LDFLAGS = -L$(JK_DIR) -ljkt -lm

#May need to do something for 32 bit alignment
$(TARGET): $(OBJS)
	gcc $(OBJS) -o $(TARGET) $(LDFLAGS)

.SUFFIXES:
.SUFFIXES:	.o .c .h

#May need to do something for 32 bit alignment
.c.o: 
	gcc -c $(CFLAGS) $*.c

jacobcalc.c: decs.h tlib.h extenv.h
linkup.c: decs.h tlib.h extenv.h
slave1.c: decs.h tlib.h extenv.h
jacobcalc2.c: decs.h tlib.h extenv.h
main.c: decs.h tlib.h mainenv.h getopt.h
slave2.c: decs.h tlib.h extenv.h
laplacalc.c: decs.h tlib.h extenv.h
multi.c: decs.h tlib.h extenv.h
subblock.c: decs.h tlib.h extenv.h
getopt.o: getopt.h

