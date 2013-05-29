#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>

#include <mntent.h>
#include <sys/stat.h>

static char *get_cgroup_mount(const char *subsystem)
{
    char *mnt = NULL;
    struct mntent mntent_r;
    FILE *file;
    char buf[BUFSIZ];

    file = setmntent("/proc/self/mounts", "r");
    if (!file)
        err(1, "failed to open mounts");

    while ((getmntent_r(file, &mntent_r, buf, sizeof(buf)))) {
        if (strcmp(mntent_r.mnt_type, "cgroup") != 0)
            continue;

        if (subsystem && !hasmntopt(&mntent_r, subsystem))
            continue;

        mnt = strdup(mntent_r.mnt_dir);
        fprintf(stderr, "using cgroup mounted at '%s'\n", mnt);
        break;
    };

    endmntent(file);
    return mnt;
}

int cgroup_subsystem(const char *subsystem)
{
    char *root = get_cgroup_mount(subsystem);
    if (root == NULL)
        return -1;

    int dirfd = open(root, O_RDONLY, FD_CLOEXEC);
    free(root);

    if (dirfd < 0)
        return -1;
    return dirfd;
}

int cgroup_controller(int cg, const char *controller)
{
    if (mkdirat(cg, controller, 0755) < 0 && errno != EEXIST)
        return -1;

    int dirfd = openat(cg, controller, O_RDONLY, FD_CLOEXEC);
    if (dirfd < 0)
        return -1;

    return dirfd;
}

int cgroup_controller_path(const char *subsystem, ...)
{
    int cg = cgroup_subsystem(subsystem);
    char *controller = NULL;
    va_list ap;

    va_start(ap, subsystem);
    while ((controller = va_arg(ap, char *))) {
        int temp = cg;
        cg = cgroup_controller(cg, controller);
        close(temp);
    }
    va_end(ap);

    return cg;
}

int subsystem_set(int cg, const char *device, const char *value)
{
    int fd = openat(cg, device, O_WRONLY, FD_CLOEXEC);
    if (fd < 0)
        return -1;

    ssize_t bytes_w = write(fd, value, strlen(value));
    close(fd);
    return bytes_w;
}

static void set_memory(void)
{
    int memory = cgroup_controller_path("memory", "playpen", NULL);
    subsystem_set(memory, "tasks", "0");
    subsystem_set(memory, "memory.limit_in_bytes", "256M");
    close(memory);
}

static void set_devices(void)
{
    int devices = cgroup_controller_path("devices", "playpen", NULL);
    subsystem_set(devices, "tasks", "0");
    subsystem_set(devices, "devices.deny", "a");
    subsystem_set(devices, "devices.allow", "c 1:9 r");
    close(devices);
}

int main(void)
{
    set_memory();
    set_devices();
}
