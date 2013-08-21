#include "dbus-systemd.h"

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "dbus.h"

void start_transient_scope(dbus_bus *bus,
                           const char *name,
                           const char *mode,
                           const char *slice,
                           const char *description,
                           dbus_message **ret)
{
    dbus_message *m;
    dbus_new_method_call("org.freedesktop.systemd1",
                         "/org/freedesktop/systemd1",
                         "org.freedesktop.systemd1.Manager",
                         "StartTransientUnit", &m);

    uint32_t pid = getpid();
    uint64_t mem = 1024 * 1024 * 128;

    dbus_message_append(m, "ss", name, mode);

    dbus_open_container(m, 'a', "(sv)");
    dbus_message_append(m, "(sv)", "Description",  "s",  description);
    dbus_message_append(m, "(sv)", "Slice",        "s",  slice);
    dbus_message_append(m, "(sv)", "PIDs",         "au", 1, pid);
    dbus_message_append(m, "(sv)", "MemoryLimit",  "t",  mem);
    dbus_message_append(m, "(sv)", "DevicePolicy", "s",  "strict");
    dbus_message_append(m, "(sv)", "DeviceAllow",  "a(ss)", 3,
                                "/dev/urandom", "r",
                                "/dev/random",  "r",
                                "/dev/null",    "rw");
    dbus_close_container(m);

    dbus_send_with_reply_and_block(bus, m, ret);
    dbus_message_free(m);
}

void get_unit(dbus_bus *bus, const char *name, dbus_message **ret)
{
    dbus_message *m;
    dbus_new_method_call("org.freedesktop.systemd1",
                         "/org/freedesktop/systemd1",
                         "org.freedesktop.systemd1.Manager",
                         "GetUnit", &m);

    dbus_message_append(m, "s", name);
    dbus_send_with_reply_and_block(bus, m, ret);
    dbus_message_free(m);
}
