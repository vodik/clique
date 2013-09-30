#include "dbus-shim.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* {{{ parsing */
static bool bus_type_is_basic(char c) {
        static const char valid[] = {
                DBUS_TYPE_BYTE,
                DBUS_TYPE_BOOLEAN,
                DBUS_TYPE_INT16,
                DBUS_TYPE_UINT16,
                DBUS_TYPE_INT32,
                DBUS_TYPE_UINT32,
                DBUS_TYPE_INT64,
                DBUS_TYPE_UINT64,
                DBUS_TYPE_DOUBLE,
                DBUS_TYPE_STRING,
                DBUS_TYPE_OBJECT_PATH,
                DBUS_TYPE_SIGNATURE,
                DBUS_TYPE_UNIX_FD
        };

        return !!memchr(valid, c, sizeof(valid));
}

static int signature_element_length_internal(const char *s,
                                             bool allow_dict_entry,
                                             unsigned array_depth,
                                             unsigned struct_depth,
                                             size_t *l) {

    int r;

    /* assert(s); */

    if (bus_type_is_basic(*s) || *s == DBUS_TYPE_VARIANT) {
        *l = 1;
        return 0;
    }

    if (*s == DBUS_TYPE_ARRAY) {
        size_t t;

        if (array_depth >= 32)
            return -EINVAL;

        r = signature_element_length_internal(s + 1, true, array_depth+1, struct_depth, &t);
        if (r < 0)
            return r;

        *l = t + 1;
        return 0;
    }

    if (*s == '(') {
        const char *p = s + 1;

        if (struct_depth >= 32)
            return -EINVAL;

        while (*p != ')') {
            size_t t;

            r = signature_element_length_internal(p, false, array_depth, struct_depth+1, &t);
            if (r < 0)
                return r;

            p += t;
        }

        *l = p - s + 1;
        return 0;
    }

    if (*s == '{' && allow_dict_entry) {
        const char *p = s + 1;
        unsigned n = 0;

        if (struct_depth >= 32)
            return -EINVAL;

        while (*p != '}') {
            size_t t;

            if (n == 0 && !bus_type_is_basic(*p))
                return -EINVAL;

            r = signature_element_length_internal(p, false, array_depth, struct_depth+1, &t);
            if (r < 0)
                return r;

            p += t;
            n++;
        }

        if (n != 2)
            return -EINVAL;

        *l = p - s + 1;
        return 0;
    }

    return -EINVAL;
}

static int signature_element_length(const char *s, size_t *l) {
    return signature_element_length_internal(s, true, 0, 0, l);
}
/* }}} */

static char *get_session_socket(void)
{
    char *socket;
    const char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");

    if (!xdg_runtime_dir)
        return NULL;

    if (asprintf(&socket, "unix:path=%s/systemd/private", xdg_runtime_dir) < 0)
        return NULL;

    return socket;
}

int dbus_open(int type, dbus_bus **ret)
{
    char *socket;
    DBusConnection *conn;
    DBusError err;

    dbus_error_init(&err);

    if (type == DBUS_AUTO) {
        type = (getuid() == 0) ? DBUS_SYSTEM : DBUS_SESSION;
    }

    switch (type) {
    case DBUS_SESSION:
        socket = get_session_socket();

        if (!socket)
            return -1;

        conn = dbus_connection_open(socket, &err);
        free(socket);
        break;
    case DBUS_SYSTEM:
        conn = dbus_bus_get(type, &err);
        break;
    }

    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }

    if (!conn)
        return -1;

    dbus_bus *bus = malloc(sizeof(dbus_bus));
    *bus = (dbus_bus){ .conn = conn };
    *ret = bus;
    return 0;
}

void dbus_close(dbus_bus *bus)
{
    dbus_connection_unref(bus->conn);
    free(bus);
}

/* {{{ stack */
int dbus_open_container(dbus_message *m, const char type, const char *contents)
{
    if (m->pos + 1 == m->size) {
        m->size *= 2;
        m->stack = realloc(m->stack, sizeof(DBusMessageIter) * 10);
    }

    DBusMessageIter *parent = &m->stack[m->pos++];
    DBusMessageIter *child  = &m->stack[m->pos];

    switch (m->mode) {
    case MESSAGE_WRITING:
        return dbus_message_iter_open_container(parent, type, contents, child);
    case MESSAGE_READING:
        if (dbus_message_iter_get_arg_type(parent) != type)
            return -EINVAL;
        dbus_message_iter_recurse(parent, child);
    }

    return 0;
}

int dbus_close_container(dbus_message *m)
{
    DBusMessageIter *child  = &m->stack[m->pos--];
    DBusMessageIter *parent = &m->stack[m->pos];

    switch (m->mode) {
    case MESSAGE_WRITING:
        return dbus_message_iter_close_container(parent, child);
        break;
    case MESSAGE_READING:
        dbus_message_iter_next(parent);
    }

    return 0;
}
/* }}} */

static dbus_message *dbus_message_create(DBusMessage *msg, int mode)
{
    dbus_message *m = malloc(sizeof(dbus_message));
    *m = (dbus_message){
        .size  = 10,
        .stack = malloc(sizeof(DBusMessageIter) * 10),
        .pos   = 0,
        .msg   = msg,
        .mode  = mode
    };
    return m;
}

int dbus_new_method_call(const char *destination,
                         const char *path,
                         const char *interface,
                         const char *method,
                         dbus_message **ret)
{
    DBusMessage *msg = dbus_message_new_method_call(destination,
                                                    path,
                                                    interface,
                                                    method);

    if (!msg)
        return -1;

    dbus_message *m = dbus_message_create(msg, MESSAGE_WRITING);
    dbus_message_iter_init_append(msg, &m->stack[m->pos]);
    *ret = m;
    return 0;
}

void dbus_message_free(dbus_message *m)
{
    dbus_message_unref(m->msg);
    free(m->stack);
    free(m);
}

int dbus_send_with_reply_and_block(dbus_bus *bus, dbus_message *m,
                                   dbus_message **ret)
{
    DBusPendingCall *pending;
    DBusMessage *msg;

    if (!dbus_connection_send_with_reply(bus->conn, m->msg, &pending, -1))
        return -ENOMEM;
    if (NULL == pending)
        return 0;

    dbus_connection_flush(bus->conn);
    dbus_pending_call_block(pending);

    if (ret) {
        msg = dbus_pending_call_steal_reply(pending);
        if (NULL == msg) {
            fprintf(stderr, "Reply Null\n");
            exit(1);
        }

        dbus_message *r = dbus_message_create(msg, MESSAGE_READING);
        dbus_message_iter_init(msg, &r->stack[r->pos]);
        *ret = r;
    }

    dbus_pending_call_unref(pending);
    return 0;
}

/* {{{ appending */
static inline int dbus_message_append_8bytes(DBusMessageIter *iter, char type, va_list ap)
{
    uint8_t val = (uint8_t)va_arg(ap, int);
    return dbus_message_iter_append_basic(iter, type, &val);
}

static inline int dbus_message_append_16bytes(DBusMessageIter *iter, char type, va_list ap)
{
    uint16_t val = (uint16_t)va_arg(ap, int);
    return dbus_message_iter_append_basic(iter, type, &val);
}

static inline int dbus_message_append_32bytes(DBusMessageIter *iter, char type, va_list ap)
{
    uint32_t val = (uint32_t)va_arg(ap, uint32_t);
    return dbus_message_iter_append_basic(iter, type, &val);
}

static inline int dbus_message_append_64bytes(DBusMessageIter *iter, char type, va_list ap)
{
    uint64_t val = (uint64_t)va_arg(ap, uint64_t);
    return dbus_message_iter_append_basic(iter, type, &val);
}

static inline int dbus_message_append_cstring(DBusMessageIter *iter, char type, va_list ap)
{
    const char *val = va_arg(ap, const char *);
    return dbus_message_iter_append_basic(iter, type, &val);
}

int dbus_message_append_ap(dbus_message *m, const char *types, va_list ap);

static inline int dbus_message_append_array(dbus_message *m, const char **t, va_list ap)
{
    unsigned length = (unsigned)va_arg(ap, unsigned);
    char type = **t;

    size_t k;
    int r = signature_element_length(*t + 1, &k);
    if (r < 0)
        return r;

    char s[k];
    memcpy(s, *t + 1, k);
    s[k] = 0;
    *t += k;

    r = dbus_open_container(m, type, s);
    if (r < 0)
        return r;

    for (unsigned i = 0; i < length; ++i) {
        r = dbus_message_append_ap(m, s, ap);
        if (r < 0)
            return r;
    }

    return dbus_close_container(m);
}

static inline int dbus_message_append_variant(dbus_message *m, va_list ap)
{
    const char *type = va_arg(ap, const char *);
    if (!type)
        return -EINVAL;

    int r = dbus_open_container(m, 'v', type);
    if (r < 0)
        return r;

    r = dbus_message_append_ap(m, type, ap);
    if (r < 0)
        return r;

    return dbus_close_container(m);
}

static inline int dbus_message_append_struct(dbus_message *m, const char **t, va_list ap)
{
    size_t k;
    char type;
    int r = signature_element_length(*t, &k);
    if (r < 0)
        return r;

    if (**t == '(')
        type = 'r';
    if (**t == '{')
        type = 'e';

    char s[k - 1];
    memcpy(s, *t + 1, k - 2);
    s[k - 2] = 0;
    *t += k - 1;

    r = dbus_open_container(m, type, NULL);
    if (r < 0)
        return r;

    r = dbus_message_append_ap(m, s, ap);
    if (r < 0)
        return r;

    return dbus_close_container(m);
}

int dbus_message_append_ap(dbus_message *m, const char *types, va_list ap)
{
    int r;

    if (!types)
        return 0;

    const char *t;
    for (t = types; *t; ++t) {
        DBusMessageIter *iter = &m->stack[m->pos];

        switch (*t) {
        case DBUS_TYPE_BYTE:
            r = dbus_message_append_8bytes(iter, *t, ap);
            break;
        case DBUS_TYPE_INT16:
        case DBUS_TYPE_UINT16:
            r = dbus_message_append_16bytes(iter, *t, ap);
            break;
        case DBUS_TYPE_BOOLEAN:
        case DBUS_TYPE_INT32:
        case DBUS_TYPE_UINT32:
        case DBUS_TYPE_UNIX_FD:
            r = dbus_message_append_32bytes(iter, *t, ap);
            break;
        case DBUS_TYPE_INT64:
        case DBUS_TYPE_UINT64:
        case DBUS_TYPE_DOUBLE:
            r = dbus_message_append_64bytes(iter, *t, ap);
            break;
        case DBUS_TYPE_STRING:
        case DBUS_TYPE_OBJECT_PATH:
        case DBUS_TYPE_SIGNATURE:
            r = dbus_message_append_cstring(iter, *t, ap);
            break;
        case DBUS_TYPE_ARRAY:
            r = dbus_message_append_array(m, &t, ap);
            break;
        case DBUS_TYPE_VARIANT:
            r = dbus_message_append_variant(m, ap);
            break;
        case DBUS_STRUCT_BEGIN_CHAR:
        case DBUS_DICT_ENTRY_BEGIN_CHAR:
            r = dbus_message_append_struct(m, &t, ap);
            break;
        default:
            r = -EINVAL;
        }

        if (r < 0)
            return r;
    }

    return 0;
}

int dbus_message_append(dbus_message *m, const char *types, ...)
{
    va_list ap;
    int r;

    if (!m)
        return -EINVAL;
    if (!types)
        return 0;

    va_start(ap, types);
    r = dbus_message_append_ap(m, types, ap);
    va_end(ap);

    return r;
}
/* }}} */

/* {{{ reading */
int dbus_message_type(dbus_message *m)
{
    DBusMessageIter *iter = &m->stack[m->pos];
    return dbus_message_iter_get_arg_type(iter);
}

int dbus_message_read_basic(dbus_message *m, char type, void *ptr)
{
    DBusMessageIter *iter = &m->stack[m->pos];

    int t = dbus_message_iter_get_arg_type(iter);
    if (t != type)
        return -EINVAL;

    dbus_message_iter_get_basic(iter, ptr);
    dbus_message_iter_next(iter);
    return 0;
}

int dbus_message_read_ap(dbus_message *m, const char *types, va_list ap);

static inline int dbus_message_read_array(dbus_message *m, const char **t, va_list ap)
{
    unsigned length = (unsigned)va_arg(ap, unsigned);

    size_t k;
    char type = **t;
    int r = signature_element_length(*t + 1, &k);
    if (r < 0)
        return r;

    char s[k];
    memcpy(s, *t + 1, k);
    s[k] = 0;
    *t += k;

    r = dbus_open_container(m, type, NULL);
    if (r < 0)
        return r;

    for (unsigned i = 0; i < length; ++i) {
        r = dbus_message_read_ap(m, s, ap);
        if (r < 0)
            return r;
    }

    return dbus_close_container(m);
}

static inline int dbus_message_read_variant(dbus_message *m, va_list ap)
{
    const char *type = va_arg(ap, const char *);
    if (!type)
        return -EINVAL;

    int r = dbus_open_container(m, 'v', NULL);
    if (r < 0)
        return r;

    r = dbus_message_read_ap(m, type, ap);
    if (r < 0)
        return r;

    return dbus_close_container(m);
}

static inline int dbus_message_read_struct(dbus_message *m, const char **t, va_list ap)
{
    size_t k;
    char type;
    int r = signature_element_length(*t, &k);
    if (r < 0)
        return r;

    if (**t == '(')
        type = 'r';
    if (**t == '{')
        type = 'e';

    char s[k - 1];
    memcpy(s, *t + 1, k - 2);
    s[k - 2] = 0;
    *t += k - 1;

    r = dbus_open_container(m, type, NULL);
    if (r < 0)
        return r;

    r = dbus_message_read_ap(m, s, ap);
    if (r < 0)
        return r;

    return dbus_close_container(m);
}

int dbus_message_read_ap(dbus_message *m, const char *types, va_list ap)
{
    void *p;
    int r;

    if (!types)
        return 0;

    const char *t;
    for (t = types; *t; ++t) {
        switch (*t) {
        case DBUS_TYPE_BYTE:
        case DBUS_TYPE_BOOLEAN:
        case DBUS_TYPE_INT16:
        case DBUS_TYPE_UINT16:
        case DBUS_TYPE_INT32:
        case DBUS_TYPE_UINT32:
        case DBUS_TYPE_INT64:
        case DBUS_TYPE_UINT64:
        case DBUS_TYPE_DOUBLE:
        case DBUS_TYPE_STRING:
        case DBUS_TYPE_OBJECT_PATH:
        case DBUS_TYPE_SIGNATURE:
        case DBUS_TYPE_UNIX_FD:
            p = va_arg(ap, void *);
            r = dbus_message_read_basic(m, *t, p);
            break;
        case DBUS_TYPE_ARRAY:
            r = dbus_message_read_array(m, &t, ap);
            break;
        case DBUS_TYPE_VARIANT:
            r = dbus_message_read_variant(m, ap);
            break;
        case DBUS_STRUCT_BEGIN_CHAR:
        case DBUS_DICT_ENTRY_BEGIN_CHAR:
            r = dbus_message_read_struct(m, &t, ap);
            break;
        default:
            printf("reading %c unimplemented\n", *t);
            r = -EINVAL;
        }

        if (r < 0)
            return r;
    }

    return 0;
}

int dbus_message_read(dbus_message *m, const char *types, ...)
{
    va_list ap;
    int r;

    if (!m)
        return -EINVAL;
    if (!types)
        return 0;

    va_start(ap, types);
    r = dbus_message_read_ap(m, types, ap);
    va_end(ap);

    return r;
}
/* }}} */
