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

typedef struct _serial_device *SerialDevice;
typedef struct serial Serial;

struct _serial {
    SerialDevice (* init) (const char *port);
    void (* free) (SerialDevice *device);
    bool (* set_baudrate) (SerialDevice device, const uint32_t baudrate);
    void (* set_parity) (SerialDevice device,enum parity parity);
    void (* set_access_mode) (SerialDevice device, enum access_mode accessMode);
    void (* set_databits) (SerialDevice device, uint8_t databits);
    void (* set_stopbits) (SerialDevice device, uint8_t stopbits);
    void (* set_echo) (SerialDevice device, bool on);
    void (* set_handshake) (SerialDevice device, enum handshake handshake);
    uint32_t (* get_current_baudrate) (SerialDevice device);
    uint8_t (* get_databits) (SerialDevice device);
    uint8_t (* get_stopbits) (SerialDevice device);
    bool (* is_echo_on) (SerialDevice device);
    enum handshake (* get_handshake) (SerialDevice device);
    enum parity (* get_parity) (SerialDevice device);
    enum access_mode (* get_access_mode) (SerialDevice device);

};

extern const struct _serial serial;

#endif //GSMAPP_SERIAL_H
