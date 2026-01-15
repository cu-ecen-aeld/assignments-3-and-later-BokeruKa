# Simple Makefile for building writer (supports optional cross-compilation)

# When CROSS_COMPILE is provided as a toolchain prefix (e.g.
# CROSS_COMPILE=aarch64-none-linux-gnu-), the compiler will be
# set to $(CROSS_COMPILE)gcc. Otherwise the native `gcc` is used.
CROSS_COMPILE ?=

ifdef CROSS_COMPILE
CC := $(CROSS_COMPILE)gcc
else
CC ?= gcc
endif

CFLAGS ?= -std=c11 -Wall -Wextra -O2 #use C11 standard with common warnings and optimizations

SRCS = writer.c #writer.c is the only source file for my writer application
OBJS = $(SRCS:.c=.o) #substitute .c with .o for object files
TARGET = writer #name of the output binary

.PHONY: default clean #declare default and clean as phony targets

default: $(TARGET) #default target builds the writer binary

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ 

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)