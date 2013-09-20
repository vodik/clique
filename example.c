#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>

#include "dbus/dbus-shim.h"
#include "dbus/dbus-util.h"
#include "systemd-scope.h"
#include "systemd-unit.h"

static void busy_loop()
{
    while (1) {
        sleep (5);
    }
}

int main(void)
{
    int rc;
    char *path, *state;
    dbus_bus *bus;
    dbus_open(DBUS_AUTO, &bus);

    pid_t pid = fork();
    if (pid == 0) {
        busy_loop();
    }

    printf("pid: %ld\n", (long)pid);

    dbus_message *scope;
    scope_init(&scope, "clique.scope", NULL, "transient unit test", pid);
    scope_memory_limit(scope, 1024 * 1024 * 128);
    scope_commit(bus, scope, NULL);

    /* char *path; */

    /* get the assiociated unit */
    rc = get_unit_by_pid(bus, pid, &path);
    if (rc < 0) {
        printf("failed to get unit path: %s\n", bus->error);
        return 1;
    }

    printf("PATH: %s\n", path);

    /* check state */
    get_unit_state(bus, path, &state);
    printf("State: %s\n", state);

    /* kill */
    getchar();
    unit_kill(bus, path, SIGTERM);

    /* check state */
    get_unit_state(bus, path, &state);
    printf("State: %s\n", state);
}
