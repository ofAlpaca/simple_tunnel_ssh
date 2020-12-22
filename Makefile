CC = gcc
CFLAGS = -g
LIB = -lssh
TARGET_LIB = libtnlc.so
PACKAGES = tnlssh $(TARGET_LIB) example

.PHONY: all

all: $(PACKAGES)

tnlssh: tnlssh.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

example: $(TARGET_LIB)
	$(CC) $(CFLAGS) -o $@ $@.c -ltnlc -L.

$(TARGET_LIB): tnlclient.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^