# List thread directories
# This must be changed for specific machines
MLTP_DIR = /home/research_home/mdipper/mltp

CC = gcc
REENTRANT = -D_REENTRANT -D__SMP__

CFLAGS = -O2 -g -Wall $(REENTRANT) -I$(MLTP_DIR)
LDFLAGS = -L$(MLTP_DIR) -lmltp -lpcl

.SUFFIXES: .c .o .s .E

all:		mcontext context

mcontext:	mcontext.c 
		$(CC) mcontext.c $(CFLAGS) $(LDFLAGS) -o mcontext

context:	context.c 
		$(CC) context.c $(CFLAGS) -lpcl -o context
