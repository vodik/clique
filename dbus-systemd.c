#include "dbus-systemd.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "dbus.h"

static inline int dbus_try_read_object(dbus_bus *bus, dbus_message *reply, const char **ret)
{
    int rc = dbus_message_read(reply, "o", ret);
    if (rc < 0)
        dbus_message_read(reply, "s", bus->error);
    return rc;
}

int start_transient_scope(dbus_bus *bus,
                          const char *name,
                          const char *mode,
                          const char *slice,
                          const char *description,
                          const char **ret)
{
    int rc = 0;
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

    dbus_message *reply;
    dbus_send_with_reply_and_block(bus, m, ret ? &reply : NULL);
    dbus_message_free(m);

    if (ret) {
        rc = dbus_try_read_object(bus, reply, ret);
        dbus_message_free(reply);
    }

    return rc;
}

int get_unit_by_pid(dbus_bus *bus, uint32_t pid, const char **ret)
{
    int rc = 0;
    dbus_message *m;
    dbus_new_method_call("org.freedesktop.systemd1",
                         "/org/freedesktop/systemd1",
                         "org.freedesktop.systemd1.Manager",
                         "GetUnitByPID", &m);

    if (pid == 0)
        pid = getpid();

    dbus_message_append(m, "u", pid);

    dbus_message *reply;
    dbus_send_with_reply_and_block(bus, m, ret ? &reply : NULL);
    dbus_message_free(m);

    if (ret) {
        rc = dbus_try_read_object(bus, reply, ret);
        dbus_message_free(reply);
    }

    return rc;
}

int get_unit(dbus_bus *bus, const char *name, const char **ret)
{
    int rc = 0;
    dbus_message *m;
    dbus_new_method_call("org.freedesktop.systemd1",
                         "/org/freedesktop/systemd1",
                         "org.freedesktop.systemd1.Manager",
                         "GetUnit", &m);

    dbus_message_append(m, "s", name);

    dbus_message *reply;
    dbus_send_with_reply_and_block(bus, m, ret ? &reply : NULL);
    dbus_message_free(m);

    if (ret) {
        rc = dbus_try_read_object(bus, reply, ret);
        dbus_message_free(reply);
    }

    return rc;
}
