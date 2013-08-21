#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "dbus.h"
#include "dbus-systemd.h"

int main(void)
{
    dbus_bus *bus;
    dbus_message *reply;
    const char *path = "/org/freedesktop/systemd1/unit/gpg_2dagent_2d1_2escope";

    dbus_open(DBUS_BUS_SYSTEM, &bus);
    start_transient_scope(bus, "gpg-agent-1.scope",
                          "fail",
                          "user-1000.slice",
                          "transient unit test",
                          &reply);
    dbus_message_read(reply, "o",  &path);
    printf("REPLY: %s\n", path);
    dbus_message_free(reply);

    get_unit(bus, "gpg-agent-1.scope", &reply);
    dbus_message_read(reply, "o",  &path);
    printf("REPLY: %s\n", path);
    dbus_message_free(reply);

    dbus_message *m;
    dbus_new_method_call("org.freedesktop.systemd1",
                         path,
                         "org.freedesktop.DBus.Properties",
                         "GetAll", &m);

    dbus_message_append(m, "s", "org.freedesktop.systemd1.Unit");

    dbus_send_with_reply_and_block(bus, m, &reply);
    dbus_message_free(m);

    int type = dbus_message_type(reply);
    if (type == 's') {
        const char *error;
        dbus_message_read(reply, "s", &error);
        printf("error: %s\n", error);
        return -1;
    }

    printf("\nPROPERTIES!\n");
    dbus_open_container(reply, 'a', NULL);

    while (1) {
        const char *key;
        union {
            const char *str;
            uint8_t u8;
            uint16_t u16;
            uint32_t u32;
            uint64_t u64;
            int16_t i16;
            int32_t i32;
            int64_t i64;
            double dbl;
        } data;

        if (dbus_open_container(reply, 'e', NULL) < 0)
            break;

        if (dbus_message_read(reply, "s", &key) < 0)
            break;

        dbus_open_container(reply, 'v', NULL);

        int type = dbus_message_type(reply);
        switch (type) {
        case DBUS_TYPE_UINT16:
            if (dbus_message_read_basic(reply, type, &data.u16) == 0)
                printf(" %s[%c]: %hu\n", key, type, data.u16);
            break;
        case DBUS_TYPE_BOOLEAN:
            if (dbus_message_read_basic(reply, type, &data.i32) == 0)
                printf(" %s[%c]: %d\n", key, type, data.i32);
            break;
        case DBUS_TYPE_UINT32:
            if (dbus_message_read_basic(reply, type, &data.u32) == 0)
                printf(" %s[%c]: %u\n", key, type, data.u32);
            break;
        case DBUS_TYPE_UINT64:
            if (dbus_message_read_basic(reply, type, &data.u64) == 0)
                printf(" %s[%c]: %lu\n", key, type, data.u64);
            break;
        case DBUS_TYPE_BYTE:
            if (dbus_message_read_basic(reply, type, &data.u8) == 0)
                printf(" %s[%c]: %c\n", key, type, data.u8);
            break;
        case DBUS_TYPE_INT16:
            if (dbus_message_read_basic(reply, type, &data.i16) == 0)
                printf(" %s[%c]: %hd\n", key, type, data.i16);
            break;
        case DBUS_TYPE_INT32:
            if (dbus_message_read_basic(reply, type, &data.i32) == 0)
                printf(" %s[%c]: %d\n", key, type, data.i32);
            break;
        case DBUS_TYPE_UNIX_FD:
            if (dbus_message_read_basic(reply, type, &data.i32) == 0)
                printf(" %s[%c]: %d\n", key, type, data.i32);
            break;
        case DBUS_TYPE_INT64:
            if (dbus_message_read_basic(reply, type, &data.i64) == 0)
                printf(" %s[%c]: %ld\n", key, type, data.i64);
            break;
        case DBUS_TYPE_DOUBLE:
            if (dbus_message_read_basic(reply, type, &data.dbl) == 0)
                printf(" %s[%c]: %f\n", key, type, data.dbl);
            break;
        case DBUS_TYPE_STRING:
        case DBUS_TYPE_OBJECT_PATH:
        case DBUS_TYPE_SIGNATURE:
            if (dbus_message_read_basic(reply, type, &data.str) == 0)
                printf(" %s[%c]: %s\n", key, type, data.str);
            break;
        default:
            printf(" %s[%c]: ???\n", key, type);
        }

        dbus_close_container(reply);
        dbus_close_container(reply);
    }

    dbus_message_free(reply);
    dbus_close(bus);
    getchar();
}
