# $Id: Makefile,v 1.1 2000/08/13 19:21:07 mdipper Exp $
#
# $Log: Makefile,v $
# Revision 1.1  2000/08/13 19:21:07  mdipper
# Initial revision
#
# Revision 1.9  2000/06/15 13:30:21  mdipper
# Remove mytp references, files have been moved to a separate mltp directory.
# Correct clean to work on Linux vs. DOS.
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
# Write to support MIngw and Linux.
#
#
CC		= gcc -Wall

.SUFFIXES: .c .o .s .E

# Linux reentrant code indication
REENTRANT	= -D_REENTRANT -D__SMP__

#
# Need to include from the current directory because "qt.h"
# will include <qtmd.h>.
#
CFLAGS		= -I. -O2 $(REENTRANT)

HDRS		= qt.h qtmd.h

LDFLAGS		= $(CFLAGS)

LINKER		= $(CC)

MAKEFILE	= Makefile

M		= Makefile

QTOBJS		= qt.o qtmds.o

SRCS		= qt.c qtmds.s

.DEFAULT:
		echo "nothing"

.c.E:		force
		$(CC) $(CFLAGS) -E $*.c > $*.E

all:		libqt.a $(M)

libqt.a:	$(QTOBJS) $(M)
		ar crv libqt.a $(QTOBJS)
		ranlib libqt.a

clean:
		rm *.o
		rm *.a

QT_H =		qt.h $(QTMD_H)
QTMD_H =	qtmd.h

###
qt.o: $(M) qt.c $(QT_H)
qtmds.o: $(M) qtmds.s
