    //
// Created by amin on 12/19/23.
//

#ifndef GSMAPP_SERIAL_H
#define GSMAPP_SERIAL_H

#include <stdbool.h>
#include <stdint.h>

enum parity {
    PARITY_NONE,
    PARITY_ODD,
    PARITY_EVEN
};

enum access_mode {
    ACCESS_READ_ONLY  = 0,
    ACCESS_WRITE_ONLY = 1,
    ACCESS_READ_WRITE = 2,
    ACCESS_NONE       = 255
};

enum handshake {
    HANDSHAKE_NONE,
    HANDSHAKE_SOFTWARE,
    HANDSHAKE_HARDWARE,
    HANDSHAKE_BOTH
};

typedef struct _serial_data *SerialData;
typedef struct serial Serial;

struct serial {
    SerialData data;

    bool (* set_baudrate) (Serial *serial, const uint32_t baudrate);
    void (* set_parity) (Serial *serial,enum parity parity);
    void (* set_access_mode) (Serial *serial, enum access_mode accessMode);
    void (* set_databits) (Serial *serial, uint8_t databits);
    void (* set_stopbits) (Serial *serial, uint8_t stopbits);
    void (* set_echo) (Serial *serial, bool on);
    void (* set_handshake) (Serial *serial, enum handshake handshake);
    uint32_t (* get_current_baudrate) (Serial serial);
    uint8_t (* get_databits) (Serial serial);
    uint8_t (* get_stopbits) (Serial serial);
    bool (* is_echo_on) (Serial serial);
    enum handshake (* get_handshake) (Serial serial);
    enum parity (* get_parity) (Serial serial);
    enum access_mode (* get_access_mode) (Serial serial);

};



Serial *serial_init(const char *port);
void serial_free(Serial **serial);

#endif //GSMAPP_SERIAL_H
