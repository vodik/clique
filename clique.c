#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>

#include "dbus-lib.h"
#include "dbus-util.h"
#include "dbus-systemd.h"

static void busy_loop()
{
    while (1) {
        sleep (5);
    }
}

int main(void)
{
    int rc;
    const char *ret, *path, *state;
    const char *scope = "gpg-agent-1.scope";
    dbus_bus *bus;

    if (getuid() != 0) {
        fprintf(stderr, "needs to be run as root\n");
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0)
        busy_loop();

    printf("pid: %ld\n", (long)pid);

    dbus_open(DBUS_BUS_SYSTEM, &bus);

    /* start the transient scope */
    rc = start_transient_scope(bus, scope,
                               "user-1000.slice",
                               "transient unit test",
                               pid, &path);
    if (rc < 0) {
        printf("failed to start transient scope: %s\n", bus->error);
        _exit(1);
    }

    /* get the assiociated unit */
    rc = get_unit_by_pid(bus, pid, &ret);
    if (rc < 0) {
        printf("failed to get unit path: %s\n", bus->error);
        return 1;
    }

    path = strdup(ret);
    printf("PATH: %s\n", path);

    /* check state */
    query_property(bus, path, "org.freedesktop.systemd1.Unit",
                   "SubState", "s", &state);
    printf("State: %s\n", state);

    /* kill */
    getchar();
    unit_kill(bus, path, SIGTERM);

    /* check state */
    query_property(bus, path, "org.freedesktop.systemd1.Unit",
                   "SubState", "s", &state);
    printf("State: %s\n", state);
}
