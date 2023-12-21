//
// Created by Amin Khozaei on 12/19/23.
// amin.khozaei@gmail.com
//
#include "serial.h"

#include <termios.h>
#include <signal.h>
#include <bits/sigaction.h>
#include <string.h>
#include <stdlib.h>

static SerialDevice serial_init(const char *port);
static void serial_free(SerialDevice *device);
static bool serial_set_baudrate (SerialDevice device, const uint32_t baudrate);
static void serial_set_parity (SerialDevice device,enum parity parity);
static void serial_set_access_mode (SerialDevice device, enum access_mode accessMode);
static void serial_set_databits (SerialDevice device, uint8_t databits);
static void serial_set_stopbits (SerialDevice device, uint8_t stopbits);
static void serial_set_echo (SerialDevice device, bool on);
static void serial_set_handshake (SerialDevice device, enum handshake handshake);
static uint32_t serial_get_current_baudrate (SerialDevice device);
static uint8_t serial_get_databits (SerialDevice device);
static uint8_t serial_get_stopbits (SerialDevice device);
static bool serial_is_echo_on (SerialDevice device);
static enum handshake serial_get_handshake (SerialDevice device);
static enum parity serial_get_parity (SerialDevice device);
static enum access_mode serial_get_access_mode (SerialDevice device);
static speed_t validate_baudrate(uint32_t);


const struct _serial serial = {
        .init = &serial_init,
        .free = &serial_free,
        .set_baudrate = &serial_set_baudrate,
        .set_parity = &serial_set_parity,
        .set_access_mode = &serial_set_access_mode,
        .set_databits = &serial_set_databits,
        .set_handshake = &serial_set_handshake,
        .set_echo = &serial_set_echo,
        .set_stopbits = &serial_set_stopbits,
        .get_access_mode = &serial_get_access_mode,
        .get_current_baudrate = &serial_get_current_baudrate,
        .get_databits = &serial_get_databits,
        .get_handshake = &serial_get_handshake,
        .get_parity = &serial_get_parity,
        .get_stopbits = &serial_get_stopbits,
        .is_echo_on = &serial_is_echo_on
};
struct _serial_device{
    char                *port;
    int                 fd;
    uint8_t             access;
    struct termios      config;
    struct termios      old_config;
    struct sigaction    action;
};

SerialDevice serial_init(const char *port)
{
    size_t name_len;
    struct _serial_device *device;

    name_len = (strlen(port) + 1);
    device = calloc(sizeof (struct _serial_device), 1);
    if (device == NULL)
        return NULL;
    device->port = calloc(sizeof (uint8_t),name_len);
    strncpy(device->port, port, name_len);
    device->config.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                             | INLCR | IGNCR | ICRNL | IXON);
    device->config.c_oflag &= ~OPOST;
    device->config.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    device->config.c_cflag &= ~(CSIZE | PARENB);
    device->config.c_cflag |= CS8;
    device->config.c_cc[VTIME] = 0;
    device->config.c_cc[VMIN] = 1;

    return device;
}

void serial_free(SerialDevice *device)
{
    if ((*device) != NULL){
        if ((*device)->fd > 0)
        {
            tcsetattr((*device)->fd, TCSANOW, &((*device)->old_config));
        }
        free((*device)->port);
        (*device)->port = NULL;
        free((*device));
        *device = NULL;
    }
}

bool serial_set_baudrate (SerialDevice device, const uint32_t baudrate)
{
    int result;
    speed_t speed;

    speed = validate_baudrate(baudrate);
    if (device != NULL && speed != B0){
        result = cfsetospeed(&device->config, speed);
        return ((result == 0) && (cfsetispeed(&device->config, speed) == 0));
    }
    return false;
}

void serial_set_parity (SerialDevice device,enum parity parity)
{
    if ( device == NULL )
        return;

    device->config.c_cflag &= ~( PARENB | PARODD );
    if (parity == PARITY_EVEN)
    {
        device->config.c_cflag |= PARENB;
    }
    else if (parity == PARITY_ODD)
    {
        device->config.c_cflag |= ( PARENB | PARODD );
    }
}

void serial_set_access_mode (SerialDevice device, enum access_mode accessMode)
{
    if ( device == NULL )
        return;
    switch (accessMode)
    {
        case ACCESS_READ_ONLY:
            device->access = ACCESS_READ_ONLY;
            break;
        case ACCESS_WRITE_ONLY:
            device->access = ACCESS_WRITE_ONLY;
            break;
        case ACCESS_READ_WRITE:
            device->access = ACCESS_READ_WRITE;
            break;
        default:
            device->access = ACCESS_NONE;
    }
}

speed_t validate_baudrate(uint32_t baud)
{
    switch(baud)
    {
        case 50:
            return B50;
        case 75:
            return B75;
        case 110:
            return B110;
        case 134:
            return B134;
        case 150:
            return B150;
        case 200:
            return B200;
        case 300:
            return B300;
        case 600:
            return B600;
        case 1200:
            return B1200;
        case 2400:
            return B2400;
        case 4800:
            return B4800;
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        case 230400:
            return B230400;
        case 460800:
            return B460800;
        case 500000:
            return B500000;
        case 576000:
            return B576000;
        case 921600:
            return B921600;
        case 1000000:
            return B1000000;
        case 1152000:
            return B1152000;
        case 1500000:
            return B1500000;
        case 2000000:
            return B2000000;
            //additional non-sparc baudrates
        case 2500000:
            return B2500000;
        case 3000000:
            return B3000000;
        case 3500000:
            return B3500000;
        case 4000000:
            return B4000000;

        default:
            return B0;
    }
}