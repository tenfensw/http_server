#
# Makefile for non-Mac UNIX systems
#

CC ?= cc
LD ?= $(CC)
AR ?= ar

CFLAGS := -Wall -std=c99 -Ihttp_server -I. $(CFLAGS)
ifdef DEBUG
CFLAGS := $(CFLAGS) -DDEBUG=1 -g
endif

LIBHTTP_SERVER_TARGETS = http_server/fds.o \
                         http_server/headers.o \
                         http_server/server.o \
                         http_server/wrappers.o
LIBHTTP_SERVER_TARGET = libhttp_server.a

TARGETS = http/main.o
TARGET = http/http

all: lib cli

lib: $(LIBHTTP_SERVER_TARGET)

$(LIBHTTP_SERVER_TARGET): $(LIBHTTP_SERVER_TARGETS)
	$(AR) crs $(LIBHTTP_SERVER_TARGET) $(LIBHTTP_SERVER_TARGETS)

$(LIBHTTP_SERVER_TARGETS) $(TARGETS):
	$(CC) -c -o $@ $(CFLAGS) $(@:.o=.c)

cli: $(TARGET)

$(TARGET): $(TARGETS)
	$(CC) -o $(TARGET) $(TARGETS) $(LIBHTTP_SERVER_TARGET)

clean: distclean

distclean:
	-rm -rf *.dSYM $(TARGET) $(TARGETS) $(LIBHTTP_SERVER_TARGET) \
			$(LIBHTTP_SERVER_TARGETS)
