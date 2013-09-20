#include "dbus-util.h"
#include "dbus-shim.h"

int query_property(dbus_bus *bus, const char *path, const char *interface,
                   const char *property, const char *types, ...)
{
    int rc = 0;
    va_list ap;
    dbus_message *m, *reply;

    dbus_new_method_call("org.freedesktop.systemd1", path,
                         "org.freedesktop.DBus.Properties",
                         "Get", &m);

    dbus_message_append(m, "ss", interface, property);

    dbus_send_with_reply_and_block(bus, m, &reply);
    dbus_message_free(m);

    rc = dbus_open_container(reply, 'v', NULL);
    if (rc < 0)
        return rc;

    va_start(ap, types);
    rc = dbus_message_read_ap(reply, types, ap);
    va_end(ap);

    dbus_message_free(reply);
    return rc;
}

