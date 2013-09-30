#ifndef SYSTEMD_SCOPE
#define SYSTEMD_SCOPE

#include "dbus/dbus-shim.h"

typedef struct device_t {
    const char *device;
    const char *perm;
} device_t;

int scope_init(dbus_message **m, const char *name, const char *slice,
               const char *desc, int32_t pid);

int scope_memory_limit(dbus_message *m, int32_t limit);

int scope_commit(dbus_bus *bus, dbus_message *m, char **ret);

int scope_device_policy(dbus_message *m, const char *policy);
int scope_allow_device(dbus_message *m, size_t count, const device_t *devices);

#endif
