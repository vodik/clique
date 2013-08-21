CFLAGS := -std=c99 \
	-Wall -Wextra -pedantic \
	-D_GNU_SOURCE \
	-I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include \
	${CFLAGS}

LDLIBS = -ldbus-1

all: clique

clique: clique.o dbus-lib.o dbus-util.o dbus-systemd.o

clean:
	${RM} clique *.o

.PHONY: all clean install uninstall
