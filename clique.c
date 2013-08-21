#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>

#include "dbus.h"
#include "dbus-systemd.h"

static int query_property(dbus_bus *bus, const char *path, const char *interface,
                          const char *property, const char *types, ...)
{
    int rc = 0;
    va_list ap;
    dbus_message *m, *reply;

    dbus_new_method_call("org.freedesktop.systemd1", path,
                         "org.freedesktop.DBus.Properties",
                         "Get", &m);

    dbus_message_append(m, "ss", interface, property);

    dbus_send_with_reply_and_block(bus, m, &reply);
    dbus_message_free(m);

    rc = dbus_open_container(reply, 'v', NULL);
    if (rc < 0)
        return rc;

    va_start(ap, types);
    rc = dbus_message_read_ap(reply, types, ap);
    va_end(ap);

    dbus_message_free(reply);
    return rc;
}

int main(void)
{
    int rc;
    const char *path, *substate;
    dbus_bus *bus;

    if (getuid() != 0) {
        fprintf(stderr, "needs to be run as root\n");
        return 1;
    }

    dbus_open(DBUS_BUS_SYSTEM, &bus);
    rc = start_transient_scope(bus, "gpg-agent-1.scope",
                               "user-1000.slice",
                               "transient unit test",
                               &path);
    if (rc < 0) {
        printf("failed to start transient scope: %s\n", bus->error);
        return 1;
    }

    printf("REPLY: %s\n", path);

    rc = get_unit_by_pid(bus, 0, &path);
    if (rc < 0) {
        printf("failed to get unit path: %s\n", bus->error);
        return 1;
    }

    printf("REPLY: %s\n", path);

    query_property(bus, path, "org.freedesktop.systemd1.Unit",
                   "SubState", "s", &substate);
    printf("SubState: %s\n", substate);

    dbus_close(bus);
    getchar();
}
