#include <stdlib.h>
#include <stdio.h>

#include "dbus.h"
#include "dbus-systemd.h"

int main(void)
{
    dbus_bus *bus;
    dbus_message *reply;
    dbus_open(DBUS_BUS_SYSTEM, &bus);

    start_transient_scope(bus, "gpg-agent-1.scope",
                          "fail",
                          "user-1000.slice",
                          "transient unit test",
                          NULL);

    get_unit(bus, "gpg-agent-1.scope", &reply);

    const char *path;
    dbus_message_read(reply, "o",  &path);
    printf("REPLY: %s\n", path);

    dbus_message_free(reply);
    dbus_close(bus);
    getchar();
}
