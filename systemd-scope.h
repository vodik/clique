#ifndef SYSTEMD_SCOPE
#define SYSTEMD_SCOPE

#include "dbus/dbus-shim.h"

int scope_init(dbus_message **m, const char *name, const char *slice,
               const char *desc, int32_t pid);

int scope_memory_limit(dbus_message *m, int32_t limit);

int scope_commit(dbus_bus *bus, dbus_message *m, char **ret);

#endif
