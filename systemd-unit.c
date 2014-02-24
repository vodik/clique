#include "systemd-unit.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "dbus/dbus-shim.h"
#include "dbus/dbus-util.h"

static inline int dbus_try_read_object(dbus_bus *bus, dbus_message *reply, char **ret)
{
    int type = dbus_message_type(reply);
    const char *buf;
    int rc = 0;

    if (type == 'o') {
        rc = dbus_message_read(reply, "o", &buf);
        *ret = strdup(buf);
    } else if (type == 's') {
        rc = -1;
        dbus_message_read(reply, "s", &buf);
        free(bus->error);
        bus->error = strdup(buf);
        *ret = NULL;
    }

    return rc;
}

int get_unit_by_pid(dbus_bus *bus, uint32_t pid, char **ret)
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

int get_unit(dbus_bus *bus, const char *name, char **ret)
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

void unit_kill(dbus_bus *bus, const char *path, int32_t signal)
{
    dbus_message *m;
    dbus_new_method_call("org.freedesktop.systemd1",
                         path,
                         "org.freedesktop.systemd1.Unit",
                         "Stop", &m);

    dbus_message_append(m, "si", "fail", signal);
    dbus_send_with_reply_and_block(bus, m, NULL);
    dbus_message_free(m);
}

int get_unit_state(dbus_bus *bus, const char *path, char **ret)
{
    return query_property(bus, path, "org.freedesktop.systemd1.Unit",
                          "SubState", "s", ret);
}
