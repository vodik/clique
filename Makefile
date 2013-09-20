CFLAGS := -std=c99 \
	-Wall -Wextra -pedantic \
	-D_GNU_SOURCE \
	-I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include \
	${CFLAGS}

LDLIBS = -ldbus-1

all: example

example: example.o systemd-scope.o systemd-unit.o \
	dbus/dbus-shim.o dbus/dbus-util.o

clean:
	${RM} clique *.o

.PHONY: all clean install uninstall
