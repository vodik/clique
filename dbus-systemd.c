#include "dbus-systemd.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "dbus-lib.h"

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

////////////////////////

int scope_init(dbus_message **m, const char *name, const char *slice,
               const char *desc, int32_t pid)
{
    /* char *path; */
    dbus_new_method_call("org.freedesktop.systemd1",
                         "/org/freedesktop/systemd1",
                         "org.freedesktop.systemd1.Manager",
                         "StartTransientUnit", m);

    dbus_message_append(*m, "ss", name, "fail");
    dbus_open_container(*m, 'a', "(sv)");

    if (desc)
        dbus_message_append(*m, "(sv)", "Description", "s", desc);
    if (slice)
        dbus_message_append(*m, "(sv)", "Slice", "s", slice);

    dbus_message_append(*m, "(sv)", "PIDs", "au", 1, pid ? pid : getpid());

    return 0;
}

int scope_memory_limit(dbus_message *m, int32_t limit)
{
    dbus_message_append(m, "(sv)", "MemoryLimit", "t", limit);
    return 0;
}

int scope_commit(dbus_bus *bus, dbus_message *m, char **ret)
{
    int rc = 0;

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
