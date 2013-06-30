#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <mntent.h>
#include <sys/stat.h>

#include "cgroups.h"

int main(void)
{
    char *namespace = NULL;
    asprintf(&namespace, "session:%d", getpid());

    int memory = cg_open_controller("memory", "playpen", namespace, NULL);
    subsystem_set(memory, "tasks", "0");
    subsystem_set(memory, "memory.limit_in_bytes", "256M");
    close(memory);

    int devices = cg_open_controller("devices", "playpen", namespace, NULL);
    subsystem_set(devices, "tasks", "0");
    subsystem_set(devices, "devices.deny", "a");
    subsystem_set(devices, "devices.allow", "c 1:9 r");
    close(devices);

    char *path = cg_get_path("memory", "playpen", namespace, NULL);
    printf("PATH: %s\n", path);

    free(path);
    free(namespace);

    /* if (cg_destroy_controller("memory", "playpen", NULL) < 0) */
    /*     err(1, "destroying controller failed"); */

    sleep(5);
}
