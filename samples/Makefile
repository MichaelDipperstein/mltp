# $Id: $
#
# $Log: $
CC		= gcc -g -Wall

.SUFFIXES: .c .o .s .E

# Linux reentrant code indication
REENTRANT	= -D_REENTRANT -D__SMP__

#
# Be sure to include paths to supporting thread packages
#
CFLAGS		= -I.. -O2 $(REENTRANT)

LDFLAGS		= $(CFLAGS)

#
# Be sure to include paths to supporting thread packages
#
LIBS		= -L.. -lmltp

LINKER		= $(CC)

.DEFAULT:
		echo "nothing"

.c.E:		force
		$(CC) $(CFLAGS) -E $*.c > $*.E

all:		mltptest mltppi atomic mdyn_mm mfix_mm locks cond

mltptest:	mltptest.c ../libmltp.a
		$(CC) mltptest.c $(CFLAGS) $(LIBS) -o mltptest

mltppi:		mltppi.c ../libmltp.a
		$(CC) mltppi.c $(CFLAGS) $(LIBS) -o mltppi

atomic:		atomic.c ../libmltp.a
		$(CC) atomic.c $(CFLAGS) $(LIBS) -o atomic

mdyn_mm:	mdyn_mm.c ../libmltp.a
		$(CC) mdyn_mm.c $(CFLAGS) $(LIBS) -o mdyn_mm

mfix_mm:	mfix_mm.c ../libmltp.a
		$(CC) mfix_mm.c $(CFLAGS) $(LIBS) -o mfix_mm

locks:	locks.c ../libmltp.a
		$(CC) locks.c $(CFLAGS) $(LIBS) -o locks

cond:	cond.c ../libmltp.a
		$(CC) cond.c $(CFLAGS) $(LIBS) -o cond

clean:
		rm *.o
