#ifndef DBUS_UTIL_H
#define DBUS_UTIL_H

#include "dbus-lib.h"

int query_property(dbus_bus *bus, const char *path, const char *interface,
                   const char *property, const char *types, ...);

#endif
