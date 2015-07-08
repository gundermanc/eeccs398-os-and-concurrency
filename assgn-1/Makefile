#
# EECS 338 Operating Systems Makefile
# Case Western Reserve University
# (C) 2015 Christian Gunderman
#
#CC = cc
CFLAGS=-Wall
OUTFILE=app

# Debug flags
DFLAGS=-g -DDEBUG

# Release flags
RFLAGS=

# Targets to build
SOURCES=main.c

.PHONY: all
all: CFLAGS+=$(RFLAGS)
all: link

.PHONY: all-debug
all-debug: CFLAGS+=$(DFLAGS)
all-debug: link

.PHONY: clean
clean:
	$(RM) $(SRCDIR)/*~
	$(RM) $(INCDIR)/*~
	$(RM) *~
	$(RM) *.o
	$(RM) $(OUTFILE)

link:
	$(CC) $(CFLAGS) $(SOURCES) -o $(OUTFILE)
