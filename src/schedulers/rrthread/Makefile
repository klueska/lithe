# Makefile for rruthread library

V ?= @
SCHEDNAME = rrthread

LIBINCDIR ?= $(PWD)/../..
LIBOBJDIRS ?= $(patsubst %, $(LIBINCDIR)/../obj/%, $(LIBDEPS))
SCHEDSDIR ?= $(LIBINCDIR)/schedulers
TESTSDIR ?= $(SCHEDSDIR)/tests

# Needed to point to installation of the scheduler directory where the
# Makevars file is located.
include $(SCHEDSDIR)/Makevars

# Needed to point to installation of the scheduler directory where the
# Maketargets file is located.
include $(SCHEDSDIR)/Maketargets

# Needed to point to installation of the scheduler directory where the 
# tests subfolder is located, to build all bthread tests for this scheduler
include $(TESTSDIR)/Makerules

