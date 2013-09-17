#ifndef DBUS_SYSTEMD
#define DBUS_SYSTEMD

#include "dbus-lib.h"

int scope_init(dbus_message **m, const char *name, const char *slice,
               const char *desc, int32_t pid);

int scope_memory_limit(dbus_message *m, int32_t limit);

int scope_commit(dbus_bus *bus, dbus_message *m, char **ret);

int get_unit_by_pid(dbus_bus *bus, uint32_t pid, char **ret);
int get_unit(dbus_bus *bus, const char *name, char **ret);

void unit_kill(dbus_bus *bus, const char *path, int32_t signal);

#endif
