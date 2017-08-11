#List thread directories
MLTP_DIR = /home/research_home/mdipper/mltp
JK_DIR = $(MLTP_DIR)/jkthreads

CC = gcc
REENTRANT = -D_REENTRANT -D__SMP__

MCFLAGS = -O2 -g -Wall $(REENTRANT) -I$(MLTP_DIR)
MLDFLAGS = -L$(MLTP_DIR) -lmltp
JCFLAGS = -O2 -g -Wall $(REENTRANT) -I$(JK_DIR)
JLDFLAGS = -L$(JK_DIR) -ljkt

.SUFFIXES: .c .o .s .E

all:		jkpi mltppi

jkpi:	jkpi.c
		$(CC) jkpi.c $(JCFLAGS) $(JLDFLAGS) -o jkpi

mltppi:	mltppi.c
		$(CC) mltppi.c $(MCFLAGS) $(MLDFLAGS) -o mltppi
