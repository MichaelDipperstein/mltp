# $Id: Makefile,v 1.2 2000/08/16 14:17:06 mdipper Exp $
#
# $Log: Makefile,v $
# Revision 1.2  2000/08/16 14:17:06  mdipper
# Make subdirectories as well as main directory
#
# Revision 1.1  2000/08/13 19:07:38  mdipper
# Initial revision
#
# Revision 1.3  2000/06/26 14:25:16  mdipper
# Replace the test program swaps with locks.
#
# Revision 1.2  2000/06/15 13:27:30  mdipper
# Correct dependencies and clean command.
#
# Revision 1.1  2000/06/14 14:39:49  mdipper
# Initial revision
#
# Revision 1.8  2000/06/12 19:56:46  mdipper
# Add directions for building context switching benchmarks.
#
# Revision 1.7  2000/05/29 16:19:35  mdipper
# Add mdyn_mm Matrix Multiplication test
#
# Revision 1.6  2000/04/27 19:31:10  mdipper
# Add rule for atomic.c delete pthread stuff.
#
# Revision 1.5  2000/04/14 14:35:15  mdipper
# Add jkthreads library to libraries linked against.
#
# Revision 1.4  2000/04/14 14:23:05  mdipper
# Remove pthread dependent code
#
# Revision 1.3  2000/03/30 15:18:40  mdipper
# Add __SMP__ definition to the command line of gcc.
#
# Revision 1.2  2000/03/16 15:28:58  mdipper
# Add support for virtual processors.
# Clean-up build process.
# Write to support Mingw and Linux.
#
#
CC		= gcc -g -Wall

.SUFFIXES: .c .o .s .E

# Linux reentrant code indication
REENTRANT	= -D_REENTRANT -D__SMP__

#
# Be sure to include paths to supporting thread packages
#
CFLAGS		= -I. -O2 $(REENTRANT)

LDFLAGS		= $(CFLAGS)

#
# Be sure to include paths to supporting thread packages
#
LIBS		= -L. -lmltp

LINKER		= $(CC)

MLOBJS		= mltp.o mltpmd.o

QTOBJS      = qt/qt.o qt/qtmds.o

JKOBJS		= jkthreads/jkcthread.o jkthreads/memalloc.o jkthreads/fileio.o

.c.E:		force
		$(CC) $(CFLAGS) -E $*.c > $*.E

all:		qt jkthreads libmltp.a samples

qt:			force
		make -C qt

jkthreads:	force
		make -C jkthreads

libmltp.a:	$(MLOBJS) $(JKOBJS) $(QTOBJS)
		ar crv libmltp.a $(MLOBJS) $(JKOBJS) $(QTOBJS)
		ranlib libmltp.a

samples:	force
		make -C samples

# Statement to force updates
force:

clean:
		rm *.o
		rm *.a

DEP_H =	qt/qtmd.h qt/qt.h jkthreads/jkcthread.h

###
mltpmd.o: $(M) mltpmd.h mltpmd.c
mltp.o: $(M) mltp.c $(DEP_H) mltp.h
