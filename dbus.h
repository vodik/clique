#ifndef CLIQUE_DBUS_H
#define CLIQUE_DBUS_H

#include <dbus/dbus.h>

enum mode {
    MESSAGE_READING,
    MESSAGE_WRITING
};

typedef struct dbus_bus_t {
    DBusConnection *conn;
} dbus_bus;

typedef struct dbus_message_t {
    DBusMessage *msg;

    int mode;
    DBusMessageIter *stack;
    size_t size;
    size_t pos;
} dbus_message;

int dbus_open(int type, dbus_bus **ret);
void dbus_close(dbus_bus *bus);

int dbus_new_method_call(const char *destination,
                         const char *path,
                         const char *interface,
                         const char *method,
                         dbus_message **ret);
void dbus_message_free(dbus_message *m);

int dbus_open_container(dbus_message *m, const char type, const char *contents);
int dbus_close_container(dbus_message *m);

int dbus_message_append_ap(dbus_message *m, const char *types, va_list ap);
int dbus_message_append(dbus_message *m, const char *types, ...);

int dbus_message_type(dbus_message *m);
int dbus_message_read_basic(dbus_message *m, char type, void *ptr);
int dbus_message_read(dbus_message *m, const char *types, ...);

int dbus_send_with_reply_and_block(dbus_bus *bus, dbus_message *m,
                                   dbus_message **ret);

#endif

