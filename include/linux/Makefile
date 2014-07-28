#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

CC ?= gcc
CFLAGS ?= -m32 --std=gnu99

SOURCES = $(shell find src/ -name "*.c")

# Disable Make's builtin bullshit.
.SECONDARY:

.PHONY: default
default: libcamkes.a

libcamkes.a: $(SOURCES:src/%.c=%.o)
	ar r "$@" $^

%.o: src/%.c
	${CC} ${CFLAGS} -c -I . -o "$@" "$<"

.PHONY: clean
clean:
	rm -f libcamkes.a *.o
