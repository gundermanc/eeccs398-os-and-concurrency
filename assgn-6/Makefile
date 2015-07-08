#
# EECS 338 Operating Systems Makefile
# Case Western Reserve University
# (C) 2015 Christian Gunderman
#
CP=cp
CLIDIR=client
SRVDIR=server

.PHONY: all
all: subproj

.PHONY: all-debug
all-debug: subproj-debug

.PHONY: clean
clean:
	$(MAKE) -C $(CLIDIR) clean
	$(MAKE) -C $(SRVDIR) clean

subproj:
	$(MAKE) -C $(CLIDIR) all
	$(MAKE) -C $(SRVDIR) all

subproj-debug:
	$(MAKE) -C $(CLIDIR) all-debug
	$(MAKE) -C $(SRVDIR) all-debug
