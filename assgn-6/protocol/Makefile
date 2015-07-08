#
# EECS 338 Operating Systems Makefile
# Case Western Reserve University
# (C) 2015 Christian Gunderman
#
#CC = cc

RPCFLAGS=
.PHONY: all
all: rpcprotocol

.PHONY: all-debug
all-debug: rpcprotocol

.PHONY: clean
clean:
	$(RM) *~
	$(RM) *.o
	$(RM) proto*.h
	$(RM) proto*.c

rpcprotocol:
	rpcgen $(RPCFLAGS) proto.x
