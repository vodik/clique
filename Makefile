CC = clang
CFLAGS = -std=c99 -O2 -Wmost -D_GNU_SOURCE
LDFLAGS = -Wl,--as-needed

test: test.c cgroups.c
