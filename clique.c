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

/* loosely adopted from systemd shared/mkdir.c */
static int mkdir_parents(const char *path, mode_t mode)
{
    struct stat st;
    const char *p, *e;

    /* return immedately if directory exists */
    if (stat(path, &st) >= 0) {
        if ((st.st_mode & S_IFMT) == S_IFDIR)
            return 0;
        else
            return -ENOTDIR;
    }

    /* create every parent directory in the path, except the last component */
    p = path + strspn(path, "/");
    for (;;) {
        int r;
        char *t;

        e = p + strcspn(p, "/");
        p = e + strspn(e, "/");

        t = strndup(path, e - path);
        if (!t)
            return -ENOMEM;

        r = mkdir(t, mode);
        free(t);

        if (r < 0 && errno != EEXIST) {
            printf("ERROR!\n");
            return -errno;
        }

        /* Is this the last component? If so, then we're * done */
        if (*p == 0)
            return 0;
    }
}

/* loosely adopted from systemd shared/util.c */
static char *vjoinpath(const char *root, va_list ap)
{
    size_t len;
    char *ret, *p;
    const char *temp;

    va_list aq;
    va_copy(aq, ap);

    if (!root)
        return NULL;

    len = strlen(root);
    while ((temp = va_arg(ap, const char *))) {
        size_t temp_len = strlen(temp) + 1;
        if (temp_len > ((size_t) -1) - len) {
            return NULL;
        }

        len += temp_len;
    }

    ret = malloc(len + 1);
    if (ret) {
        p = stpcpy(ret, root);
        while ((temp = va_arg(aq, const char *))) {
            p++[0] = '/';
            p = stpcpy(p, temp);
        }
    }

    return ret;
}

/* static char *joinpath(const char *root, ...) */
/* { */
/*     va_list ap; */
/*     char *ret; */

/*     va_start(ap, root); */
/*     ret = vjoinpath(root, ap); */
/*     va_end(ap); */

/*     return ret; */
/* } */

static char *cg_get_mount(const char *subsystem)
{
    char *mnt = NULL;
    struct mntent mntent_r;
    FILE *file;
    char buf[BUFSIZ];

    file = setmntent("/proc/self/mounts", "r");
    if (!file)
        return NULL;

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

static char *cg_path(const char *subsystem, va_list ap)
{
    char *root, *path;

    root = cg_get_mount(subsystem);
    path = vjoinpath(root, ap);

    free(root);
    return path;
}

char *cg_get_path(const char *subsystem, ...)
{
    va_list ap;
    char *path;

    va_start(ap, subsystem);
    path = cg_path(subsystem, ap);
    va_end(ap);

    return path;
}

int cg_open_subsystem(const char *subsystem)
{
    char *root = cg_get_mount(subsystem);
    if (root == NULL)
        return -1;

    int dirfd = open(root, O_RDONLY, FD_CLOEXEC);
    free(root);

    if (dirfd < 0)
        return -1;
    return dirfd;
}

int cg_open_controller(const char *subsystem, ...)
{
    va_list ap;
    char *path;

    va_start(ap, subsystem);
    path = cg_path(subsystem, ap);
    va_end(ap);

    int rc = mkdir_parents(path, 0755);
    if (rc < 0)
        return -1;

    int dirfd = open(path, O_RDONLY, FD_CLOEXEC);
    free(path);

    if (dirfd < 0)
        return -1;
    return dirfd;
}

int cg_destroy_controller(const char *subsystem, ...)
{
    va_list ap;
    char *path;

    va_start(ap, subsystem);
    path = cg_path(subsystem, ap);
    va_end(ap);

    int rc = rmdir(path);

    free(path);
    return rc;
}

int cg_open_subcontroller(int cg, const char *controller)
{
    if (mkdirat(cg, controller, 0755) < 0 && errno != EEXIST)
        return -1;

    int dirfd = openat(cg, controller, O_RDONLY, FD_CLOEXEC);
    if (dirfd < 0)
        return -1;

    return dirfd;
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

FILE *subsystem_open(int cg, const char *device, const char *mode)
{
    int fd = openat(cg, device, O_RDWR, FD_CLOEXEC);
    if (fd < 0)
        return NULL;

    return fdopen(fd, mode);
}

int main(void)
{
    char *namespace;
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

    sleep(40);
}
