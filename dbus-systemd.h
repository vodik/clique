#ifndef DBUS_SYSTEMD
#define DBUS_SYSTEMD

#include "dbus-lib.h"

int start_transient_scope(dbus_bus *bus,
                          const char *name,
                          const char *slice,
                          const char *description,
                          uint32_t pid,
                          const char **ret);

int get_unit_by_pid(dbus_bus *bus, uint32_t pid, const char **ret);
int get_unit(dbus_bus *bus, const char *name, const char **ret);

void unit_kill(dbus_bus *bus, const char *path, int32_t signal);

#endif
