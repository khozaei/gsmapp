//
// Created by Amin Khozaei on 12/19/23.
//amin.khozaei@gmail.com
//
#include "serial.h"

#include <termios.h>
#include <signal.h>
#include <bits/sigaction.h>
#include <string.h>
#include <stdlib.h>

static bool _serial_set_baudrate (Serial *serial, const uint32_t baudrate);
static speed_t validate_baudrate(uint32_t);
struct _serial_data{
    char                *port;
    int                 fd;
    struct termios      config;
    struct termios      old_config;
    struct sigaction    action;
};

Serial *serial_init(const char *port)
{
    size_t name_len;
    Serial *serial;

    name_len = (strlen(port) + 1);
    serial = calloc(sizeof (Serial), 1);
    if (serial == NULL)
        return NULL;
    serial->data = calloc(sizeof (struct _serial_data), 1);
    if (serial->data == NULL){
        free(serial);
        serial = NULL;
    }
    serial->data->port = calloc(sizeof (uint8_t),name_len);
    strncpy(serial->data->port, port, name_len);
    serial->data->config.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                             | INLCR | IGNCR | ICRNL | IXON);
    serial->data->config.c_oflag &= ~OPOST;
    serial->data->config.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    serial->data->config.c_cflag &= ~(CSIZE | PARENB);
    serial->data->config.c_cflag |= CS8;
    serial->data->config.c_cc[VTIME] = 0;
    serial->data->config.c_cc[VMIN] = 1;

    serial->set_baudrate = &_serial_set_baudrate;
    return serial;
}

void serial_free(Serial **serial)
{
    if ((*serial) != NULL){
        if ((*serial)->data != NULL){
            if ((*serial)->data->fd > 0)
            {
                tcsetattr((*serial)->data->fd, TCSANOW, &((*serial)->data->old_config));
            }
            free((*serial)->data->port);
            (*serial)->data->port = NULL;
            free((*serial)->data);
            (*serial)->data = NULL;
        }
        free((*serial));
        *serial = NULL;
    }
}

bool _serial_set_baudrate (Serial *serial, const uint32_t baudrate)
{
    int result;
    speed_t speed;

    speed = validate_baudrate(baudrate);
    if (serial != NULL && serial->data != NULL && speed != B0){
        result = cfsetospeed(&serial->data->config, speed);
        return ((result == 0) && (cfsetispeed(&serial->data->config, speed) == 0));
    }
    return false;
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