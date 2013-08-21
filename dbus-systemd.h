#ifndef DBUS_SYSTEMD
#define DBUS_SYSTEMD

#include "dbus.h"

void start_transient_scope(dbus_bus *bus,
                           const char *name,
                           const char *mode,
                           const char *slice,
                           const char *description,
                           dbus_message **ret);

void get_unit(dbus_bus *bus,
              const char *name,
              dbus_message **ret);

#endif
