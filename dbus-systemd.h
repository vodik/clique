#ifndef DBUS_SYSTEMD
#define DBUS_SYSTEMD

#include "dbus.h"

int start_transient_scope(dbus_bus *bus,
                          const char *name,
                          const char *mode,
                          const char *slice,
                          const char *description,
                          const char **ret);

int get_unit(dbus_bus *bus,
             const char *name,
             const char **ret);

#endif
