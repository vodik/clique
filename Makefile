CC = clang
CXXFLAGS = -std=c99 -O2 -Wmost
LDFLAGS = -Wl,--as-needed

clique: clique.c
