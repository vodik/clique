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
#include <linux/limits.h>

struct cgroup {
    int dirfd;
};

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

struct cgroup *cgroup_subsystem(const char *subsystem)
{
    struct cgroup *cg = NULL;
    char *root;
    int dirfd;

    root = get_cgroup_mount(subsystem);
    if (root == NULL)
        return NULL;

    dirfd = open(root, O_RDONLY);
    if (dirfd < 0)
        goto cleanup;

    cg = malloc(sizeof(struct cgroup));
    cg->dirfd = dirfd;

cleanup:
    free(root);
    return cg;
}

struct cgroup *cgroup_controller(struct cgroup *cg, const char *controller)
{
    if (mkdirat(cg->dirfd, controller, 0755) < 0 && errno != EEXIST)
        return NULL;

    int dirfd = openat(cg->dirfd, controller, O_RDONLY);
    if (dirfd < 0)
        return NULL;

    struct cgroup *ccg = malloc(sizeof(struct cgroup));
    ccg->dirfd = dirfd;

    return ccg;
}

int subsystem_set(struct cgroup *cg, const char *device, const char *value)
{
    int fd = openat(cg->dirfd, device, O_WRONLY);
    if (fd < 0)
        return -1;

    ssize_t bytes_w = write(fd, value, strlen(value));
    close(fd);
    return bytes_w;
}

static void set_memory(void)
{
    struct cgroup *memory  = cgroup_subsystem("memory");
    struct cgroup *playpen = cgroup_controller(memory, "playpen");

    subsystem_set(playpen, "tasks", "0");
    subsystem_set(playpen, "memory.limit_in_bytes", "256M");
}

static void set_devices(void)
{
    struct cgroup *memory  = cgroup_subsystem("devices");
    struct cgroup *playpen = cgroup_controller(memory, "playpen");

    subsystem_set(playpen, "tasks", "0");
    subsystem_set(playpen, "devices.deny", "a");
    subsystem_set(playpen, "devices.allow", "c 1:9 r");
}

int main(void)
{
    set_memory();
    set_devices();

    sleep(50);
}
