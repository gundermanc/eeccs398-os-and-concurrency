#
# EECS 338 Operating Systems Makefile
# Case Western Reserve University
# (C) 2015 Christian Gunderman
#
#CC = cc
PROTODIR=../protocol
DSDIR=../extern/c-datastructs
CFLAGS=-Wall --std=c99 --pedantic -I $(PROTODIR) -I $(DSDIR)/include
OUTFILE=client
LNFLAGS=-lrt -lnsl

# Debug flags
DFLAGS=-g -DDEBUG

# Release flags
RFLAGS=

# Targets to build
SOURCES=$(PROTODIR)/proto_clnt.c $(PROTODIR)/proto_xdr.c client.c

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
	$(MAKE) -C $(PROTODIR) clean
	$(MAKE) -C $(DSDIR) clean

protocol:
	$(MAKE) -C $(PROTODIR) all

c-datastructs:
	$(MAKE) -C $(DSDIR) library

link: protocol c-datastructs
	$(CC) $(CFLAGS) $(SOURCES) -o $(OUTFILE) $(LNFLAGS)
