#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>

#include "dbus-lib.h"
#include "dbus-util.h"
#include "dbus-systemd.h"

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
