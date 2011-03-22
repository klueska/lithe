# Makefile for hard threads tests

SHELL = '/bin/sh'
EXECS = test

CC = gcc

CFLAGS = -g -O2 -Wall -I..
LDFLAGS = -L.. -lht -lpthread

ALL_CFLAGS = $(CFLAGS) -std=gnu99 -MMD -MP
ALL_LDFLAGS = $(LDFLAGS)

all: htlib $(EXECS)

htlib:
	@$(MAKE) -C ..

DEPS = $(patsubst %, %.d, $(EXECS))
-include $(DEPS)

%: %.c 
	$(CC) -o $@ $< $(ALL_CFLAGS) $(ALL_LDFLAGS)

clean:
	rm -rf $(EXECS) $(DEPS)

.PHONY: all htlib clean
