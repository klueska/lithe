# Makefile for hard threads library.
# TODO(benh): Add a configure file which checks for necessary __thread support.

SHELL = '/bin/sh'

ifndef prefix
PREFIX = /usr/local
else
PREFIX = $(prefix)
endif

CC = gcc

CFLAGS = -g -O2 -Wall -fno-strict-aliasing -DUSE_FUTEX -I.
LDFLAGS =

ALL_CFLAGS = $(CFLAGS) -std=gnu99 -MMD -MP #-ftls-model="initial-exec" 
ALL_LDFLAGS = $(LDFLAGS)

LIB_C_OBJ = ht.o tls.o

LIB_ASM_OBJ = __ht_yield.o

LIB_OBJ = $(LIB_C_OBJ) $(LIB_ASM_OBJ)

# IMPORTANT: We build a static library because
#     (a) we need TLS loads to be done without the stack
#     (b) we want our library constructor to run on the main thread
LIB = libht.a

all: $(LIB)

-include $(patsubst %.o, %.d, $(LIB_OBJ))

$(LIB_C_OBJ): %.o: %.c
	$(CC) -c $(ALL_CFLAGS) -o $@ $<

$(LIB_ASM_OBJ): %.o: %.S
	$(CC) -c $(ALL_CFLAGS) -o $@ $<

$(LIB): $(LIB_OBJ)
	$(AR) rcs $@ $^

tests: all
	$(MAKE) -C tests

install-lib:
	@echo "---------------------------------------------------------------"
	@install -m 755 -d $(PREFIX)/lib
	@install -m 755 $(LIB) $(PREFIX)/lib
	@echo 
	@echo "Libraries have been installed in:"
	@echo "  $(PREFIX)/lib    (LIBDIR)"
	@echo 
	@echo "Use -LLIBDIR to link against these libraries."
	@echo 
	@echo "You may also need to do one of the following:"
	@echo "  - add LIBDIR to 'LD_LIBRARY_PATH' environment variable"
	@echo "    during execution"
	@echo "  - add LIBDIR to 'LD_RUN_PATH' environment variable"
	@echo "    during linking"
	@echo "  - use the '-RLIBDIR' linker flag"
	@echo 
	@echo "If you have super-user permissions you can add LIBDIR to"
	@echo "/etc/ld.so.conf (if it isn't there already), and run ldconfig."
	@echo 

install-hdr:
	@echo "---------------------------------------------------------------"
	@install -m 755 -d $(PREFIX)/include/ht
	@install -m 644 htmain.h $(PREFIX)/include/ht
	@install -m 644 ht.h $(PREFIX)/include/ht
	@install -m 644 atomic.h $(PREFIX)/include/ht
	@install -m 644 spinlock.h $(PREFIX)/include/ht
	@install -m 644 mcs.h $(PREFIX)/include/ht
	@echo 
	@echo "Header files have been installed in:"
	@echo "  $(PREFIX)/include/ht    (INCLUDEDIR)"
	@echo 
	@echo "Use -IINCLUDEDIR to use these headers."
	@echo 

install: $(LIB) install-hdr install-lib

uninstall:
	$(error unimplemented)

dist:
	$(error unimplemented)

clean:
	$(MAKE) -C tests clean
	rm -f $(LIB) $(LIB_OBJ) $(patsubst %.o, %.d, $(LIB_OBJ))

.PHONY: default install-lib install-hdr install uninstall dist all clean
