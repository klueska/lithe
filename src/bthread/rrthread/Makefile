# Makefile for rrthread library

V ?= @
SCHEDNAME = rrthread

SCHEDDIR ?= $(PWD)/..
BUILDDIR ?= $(PWD)/build
BUILDINCDIR ?= $(BUILDDIR)/include
BUILDOBJDIR ?= $(BUILDDIR)/obj
BUILDLIBDIR ?= $(BUILDDIR)/lib
LIBBUILDDIR ?= $(PWD)/../../../build
LIBINCDIR ?= $(LIBBUILDDIR)/include
LIBOBJDIR ?= $(LIBBUILDDIR)/obj
LIBLIBDIR ?= $(LIBBUILDDIR)/lib
TESTSDIR ?= $(SCHEDDIR)/tests

# Needed to point to installation of the scheduler directory where the 
# Makerules file is located to build all bthread libs for this scheduler
include $(SCHEDDIR)/Makerules

# Needed to point to installation of the scheduler directory where the 
# tests subfolder is located, to build all bthread tests for this scheduler
include $(TESTSDIR)/Makerules

