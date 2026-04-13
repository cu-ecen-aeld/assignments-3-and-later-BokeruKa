# Simple Makefile for aesdsocket
# Targets: default (alias), all, clean
# Supports cross-compilation via setting CROSS_COMPILE (prefix) or CC.

TARGET := aesdsocket
SRCS := aesdsocket.c

# If CROSS_COMPILE is set (e.g. arm-linux-gnueabihf-), use it; otherwise allow CC override
ifdef CROSS_COMPILE
CC := $(CROSS_COMPILE)gcc
else
CC ?= gcc
endif

CFLAGS ?= -Wall -Wextra -g
BINDIR := .

.PHONY: default all clean

default: all

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(BINDIR)/$(TARGET)

# Usage examples:
#  make            # builds with host gcc
#  make CROSS_COMPILE=arm-linux-gnueabihf-  # cross-build using arm toolchain prefix
#  make CC=/usr/bin/aarch64-linux-gnu-gcc    # explicitly specify compiler