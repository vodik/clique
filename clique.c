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

    char *path;
    {
        DBusMessageIter args;
        int type;

        dbus_message_iter_init(reply->msg, &args);

        type = dbus_message_iter_get_arg_type(&args);
        dbus_message_iter_get_basic(&args, &path);
        printf(":: TYPE: %c: %s\n", type, path);
    }

    dbus_message_free(reply);
    dbus_close(bus);
    getchar();
}
