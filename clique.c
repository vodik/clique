#include <stdlib.h>
#include <stdio.h>

#include "dbus.h"
#include "dbus-systemd.h"

int main(void)
{
    dbus_bus *bus;
    dbus_message *reply;
    const char *path;
    const char *description;

    dbus_open(DBUS_BUS_SYSTEM, &bus);
    start_transient_scope(bus, "gpg-agent-1.scope",
                          "fail",
                          "user-1000.slice",
                          "transient unit test",
                          &reply);
    dbus_message_read(reply, "o",  &path);
    printf("REPLY: %s\n", path);
    dbus_message_free(reply);

    get_unit(bus, "gpg-agent-1.scope", &reply);
    dbus_message_read(reply, "o",  &path);
    printf("REPLY: %s\n", path);
    dbus_message_free(reply);

    {
        dbus_message *m, *ret;
        dbus_new_method_call("org.freedesktop.systemd1",
                             path,
                             "org.freedesktop.DBus.Properties",
                             /* "GetAll", */ "Get",
                             &m);

        /* dbus_message_append(m, "s", "org.freedesktop.systemd1.Unit"); */
        dbus_message_append(m, "ss",
                            "org.freedesktop.systemd1.Unit",
                            "Description");

        dbus_send_with_reply_and_block(bus, m, &ret);
        dbus_message_free(m);

        dbus_message_read(ret, "v", "s",  &description);
        printf("DESCRIPTION: %s\n", description);

        dbus_message_free(ret);
    }

    dbus_close(bus);
    getchar();
}
