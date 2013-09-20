#ifndef SYSTEMD_UNIT
#define SYSTEMD_UNIT

#include "dbus/dbus-shim.h"

int get_unit_by_pid(dbus_bus *bus, uint32_t pid, char **ret);
int get_unit(dbus_bus *bus, const char *name, char **ret);
void unit_kill(dbus_bus *bus, const char *path, int32_t signal);
int get_unit_state(dbus_bus *bus, const char *path, char **ret);

#endif
