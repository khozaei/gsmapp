//
// Created by Amin Khozaei on 12/19/23.
// amin.khozaei@gmail.com
//
#include "serial.h"

#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/epoll.h>

#define MAX_EVENTS 5
#define BUFFER_SIZE 1024

static SerialDevice serial_init(const char *port);
static void serial_free(SerialDevice *device);

static void serial_open (SerialDevice device);
static void serial_close (SerialDevice device);
static intmax_t serial_write (SerialDevice device, const uint8_t *data, size_t length);
static intmax_t serial_read (SerialDevice device,  uint8_t *data, size_t length, uint32_t  ms);
static void serial_enable_async (SerialDevice device, void (* callback) (int fd, uint8_t *data, size_t length));
static void serial_disable_async (SerialDevice device);
static void *serial_read_async(void *device);

static bool serial_set_baudrate (SerialDevice device, uint32_t baudrate);
static void serial_set_parity (SerialDevice device,enum parity parity);
static void serial_set_access_mode (SerialDevice device, enum access_mode accessMode);
static void serial_set_databits (SerialDevice device, uint8_t databits);
static void serial_set_stopbits (SerialDevice device, uint8_t stopbits);
static void serial_set_echo (SerialDevice device, bool on);
static void serial_set_handshake (SerialDevice device, enum handshake handshake);
static uint32_t serial_get_current_baudrate (SerialDevice device);
static uint8_t serial_get_databits (SerialDevice device);
static uint8_t serial_get_stopbits (SerialDevice device);
static int serial_get_file_descriptor (SerialDevice device);
static bool serial_is_echo_on (SerialDevice device);
static enum handshake serial_get_handshake (SerialDevice device);
static enum parity serial_get_parity (SerialDevice device);
static enum access_mode serial_get_access_mode (SerialDevice device);
static speed_t validate_baudrate(uint32_t);

const struct _serial serial = {
        .init = &serial_init,
        .free = &serial_free,
        .open = &serial_open,
        .close = &serial_close,
        .write = &serial_write,
        .read = &serial_read,
        .enable_async = &serial_enable_async,
        .disable_async = &serial_disable_async,
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
        .get_file_descriptor = serial_get_file_descriptor,
        .is_echo_on = &serial_is_echo_on
};


struct _serial_device{
    char                *port;
    int                 fd;
    uint8_t             access;
    struct termios      config;
    struct termios      old_config;
    pthread_t           *thread;
    void (* callback) (int fd, uint8_t *data, size_t length);
};

SerialDevice serial_init(const char *port)
{
    size_t name_len;
    struct _serial_device *device;

    name_len = (strlen(port) + 1);
    device = malloc(sizeof (struct _serial_device));
    if (device == NULL)
        return NULL;
    device->port = calloc(sizeof (uint8_t),name_len);
    strncpy(device->port, port, name_len);
    device->config.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                             | INLCR | IGNCR | ICRNL | IXON);
    device->config.c_oflag &= ~OPOST;
    device->config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
    device->config.c_lflag |= ISIG;
    device->config.c_cflag &= ~(CSIZE | PARENB);
    device->config.c_cflag |= CS8;
//    device->config.c_lflag |= ICANON | ISIG;  /* canonical input */
    device->config.c_cc[VTIME] = 0;
    device->config.c_cc[VMIN] = 1;
    device->thread = NULL;
    return device;
}

void serial_free(SerialDevice *device)
{
    if ((*device) != NULL){
        if ((*device)->fd > 0)
        {
            tcsetattr((*device)->fd, TCSANOW, &((*device)->old_config));
        }
        if ((*device)->thread != NULL)
            serial_disable_async(*device);
        free((*device)->port);
        (*device)->port = NULL;
        free((*device));
        *device = NULL;
    }
}

void serial_open (SerialDevice device)
{
    int flag;

    if (device == NULL)
        return;
    if ( device->fd > 0 )
    {
        close(device->fd);
        device->fd = -1;
    }
    switch(device->access)
    {
        case ACCESS_READ_ONLY:
            flag = (O_RDONLY |  O_NOCTTY | O_SYNC);
            break;
        case ACCESS_WRITE_ONLY:
            flag = (O_WRONLY |  O_NOCTTY | O_SYNC);
            break;
        case ACCESS_READ_WRITE:
            flag = (O_RDWR |  O_NOCTTY | O_SYNC);
            break;
        default:
            flag = (O_RDONLY |  O_NOCTTY | O_SYNC);
    }
    device->fd = open(device->port, flag);
    if (device->fd > 0)
    {
        tcgetattr(device->fd, &device->old_config);
        tcflush(device->fd, TCIOFLUSH);
        tcsetattr(device->fd, TCSANOW, &device->config);
    }
}

void serial_close (SerialDevice device)
{
    if (device == NULL)
        return;
    if ( device->fd > 0 )
    {
        tcsetattr(device->fd, TCSANOW, &device->config);
        close(device->fd);
        device->fd = -1;
    }
}

intmax_t serial_write (SerialDevice device, const uint8_t *data, size_t length)
{
    intmax_t res;

    res = 0;
    if (device == NULL)
        return 0;
    if ( device->fd > 0 )
    {
        res = write(device->fd, data, length);
        tcdrain(device->fd);
//        tcflush(device->fd, TCIOFLUSH);
//        sync();
    }
    return res;
}

intmax_t serial_read (SerialDevice device,  uint8_t *data, size_t length, uint32_t ms)
{
    intmax_t res;
    uint32_t time;

    res = 0;
    time = 0;
    memset(data, 0 , length);
    if (device == NULL)
        return 0;
    if ( device->fd > 0)
    {
        while (time <= ms) {
            res = read(device->fd, &data[0], length);
            tcflush(device->fd, TCIFLUSH);
            if (errno == EAGAIN || errno == EINTR || res == 0) {
                usleep(10 * 1000);//10ms
                time+=10;
                continue;
            }
            else {
                break;
            }
        }
    }
    return res;
}

void serial_enable_async (SerialDevice device, void (* callback) (int fd, uint8_t *data, size_t length))
{
    if (device == NULL)
        return;
    if (device->fd > 0)
    {
        if (device->thread != NULL)
            serial_disable_async(device);
        device->thread = malloc(sizeof (pthread_t));
        if (device->thread) {
            pthread_create(device->thread, NULL, serial_read_async,(void *)device);
        }
        device->callback = callback;
    }
}

void serial_disable_async (SerialDevice device)
{
    if (device == NULL)
        return;
    if (device->thread != NULL)
    {
        pthread_cancel(*device->thread);
        free(device->thread);
        device->thread = NULL;
    }
}

void *serial_read_async(void *device_void)
{
    //read thread loop
    int epfd, ready;
    ssize_t len;
    struct epoll_event ev, ev_list[MAX_EVENTS];
    uint8_t data[BUFFER_SIZE];

    SerialDevice device = (SerialDevice)device_void;

    if (device == NULL)
        return NULL;
    epfd = epoll_create(1);
    if (epfd < 0)
        return NULL;
    ev.events = EPOLLIN;
    ev.data.fd = device->fd;
    if (epoll_ctl(epfd,EPOLL_CTL_ADD,device->fd,&ev) == -1)
        return NULL;
    while (device->thread != NULL && pthread_equal(*device->thread, pthread_self())) {
        ready = epoll_wait(epfd, ev_list, MAX_EVENTS,-1);
        if (ready == -1){
            if (errno == EINTR)
                continue;
            else
                return NULL;
        }
        printf("ready: %d\n", ready);

        for (int i = 0; i < ready; i++) {
            if (ev_list[i].events & EPOLLIN) {
                len = read(device->fd, data, BUFFER_SIZE);
                if (len == -1)
                    return NULL;
                if (device->callback)
                    device->callback(device->fd, data, len+1);
            }
        }
    }

    return NULL;
}

bool serial_set_baudrate (SerialDevice device, const uint32_t baudrate)
{
    int result;
    speed_t speed;

    speed = validate_baudrate(baudrate);
    if (device != NULL && speed != B0)
    {
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

void serial_set_databits (SerialDevice device, uint8_t databits)
{
    if (device == NULL)
        return;
    switch (databits)
    {
        case 5:
            device->config.c_cflag = ( device->config.c_cflag & ~CSIZE ) | CS5;
            break;
        case 6:
            device->config.c_cflag = ( device->config.c_cflag & ~CSIZE ) | CS6;
            break;
        case 7:
            device->config.c_cflag = ( device->config.c_cflag & ~CSIZE ) | CS7;
            break;
        case 8:
        default:
            device->config.c_cflag = ( device->config.c_cflag & ~CSIZE ) | CS8;
            break;
    }
    device->config.c_cflag |= CLOCAL | CREAD;
}

void serial_set_stopbits (SerialDevice device, uint8_t stopbits)
{
    if (device == NULL)
        return;
    if ( stopbits == 2 )
    {
        device->config.c_cflag |= CSTOPB;
    }
    else
    {
        device->config.c_cflag &= ~CSTOPB;
    }
}

void serial_set_echo (SerialDevice device, bool on)
{
    if (device == NULL)
        return;
    if (on)
    {
        device->config.c_lflag |= ECHO;
    }
    else
    {
        device->config.c_lflag &= ~ECHO;
    }
}

void serial_set_handshake (SerialDevice device, enum handshake handshake)
{
    if (device == NULL)
        return;
    if ( handshake == HANDSHAKE_SOFTWARE || handshake == HANDSHAKE_BOTH)
    {
        device->config.c_iflag |= IXON | IXOFF;
    }
    else
    {
        device->config.c_iflag &= ~(IXON | IXOFF | IXANY);
    }

    if (handshake == HANDSHAKE_HARDWARE || handshake == HANDSHAKE_BOTH)
    {
        device->config.c_cflag |= CRTSCTS;
    }
    else
    {
        device->config.c_cflag &= ~CRTSCTS;
    }
}

uint32_t serial_get_current_baudrate (SerialDevice device)
{
    speed_t baud;

    if (device == NULL)
        return 0;
    if ( device->fd > 0)
    {
        struct termios tmp;
        tcgetattr(device->fd, &tmp);
        baud = cfgetispeed(&tmp);
    }
    else
    {
        baud = cfgetispeed(&device->config);
    }
    switch(baud)
    {
        case B50:
            return 50;
        case B75:
            return 75;
        case B110:
            return 110;
        case B134:
            return 134;
        case B150:
            return 150;
        case B200:
            return 200;
        case B300:
            return 300;
        case B600:
            return 600;
        case B1200:
            return 1200;
        case B2400:
            return 2400;
        case B4800:
            return 4800;
        case B9600:
            return 9600;
        case B19200:
            return 19200;
        case B38400:
            return 38400;
        case B57600:
            return 57600;
        case B115200:
            return 115200;
        case B230400:
            return 230400;
        case B460800:
            return 460800;
        case B500000:
            return 500000;
        case B576000:
            return 576000;
        case B921600:
            return 921600;
        case B1000000:
            return 1000000;
        case B1152000:
            return 1152000;
        case B1500000:
            return 1500000;
        case B2000000:
            return 2000000;
            //additional non-sparc baudrates
        case B2500000:
            return 2500000;
        case B3000000:
            return 3000000;
        case B3500000:
            return 3500000;
        case B4000000:
            return 4000000;

        default:
            return 0;
    }
}

uint8_t serial_get_databits (SerialDevice device)
{
    struct termios tmp;

    if (device == NULL)
        return 0;
    if ( device->fd > 0 )
    {
        tcgetattr(device->fd, &tmp);
    }
    else
    {
        memcpy(&tmp,&device->config,sizeof(device->config));
    }
    switch (tmp.c_cflag & CSIZE)
    {
        case CS5:
            return 5;
        case CS6:
            return 6;
        case CS7:
            return 7;
        case CS8:
            return 8;
        default:
            return 0;
    }
}

uint8_t serial_get_stopbits (SerialDevice device)
{
    struct termios tmp;

    if (device == NULL)
        return 0;
    if ( device->fd > 0 )
    {
        tcgetattr(device->fd, &tmp);
    }
    else
    {
        memcpy(&tmp,&device->config,sizeof(device->config));
    }

    return (tmp.c_cflag & CSTOPB)?2:1;
}

bool serial_is_echo_on (SerialDevice device)
{
    struct termios tmp;

    if (device == NULL)
        return false;
    if ( device->fd > 0 )
    {
        tcgetattr(device->fd, &tmp);
    }
    else
    {
        memcpy(&tmp,&device->config,sizeof(device->config));
    }

    return (tmp.c_lflag & ECHO);
}

enum handshake serial_get_handshake (SerialDevice device)
{
    struct termios tmp;
    bool software, hardware;

    software = false;
    hardware = false;
    if (device == NULL)
        return HANDSHAKE_NONE;
    if ( device->fd > 0 )
    {
        tcgetattr(device->fd, &tmp);
    }
    else
    {
        memcpy(&tmp,&device->config,sizeof(device->config));
    }

    software = ((tmp.c_iflag & (IXON | IXOFF)) != 0);
    hardware = ((tmp.c_cflag & CRTSCTS) != 0);

    return (software && hardware)?HANDSHAKE_BOTH:
        (software)?HANDSHAKE_SOFTWARE:(hardware)?HANDSHAKE_HARDWARE:HANDSHAKE_NONE;
}

enum parity serial_get_parity (SerialDevice device)
{
    struct termios tmp;

    if (device == NULL)
        return PARITY_NONE;
    if ( device->fd > 0 )
    {
        tcgetattr(device->fd, &tmp);
    }
    else
    {
        memcpy(&tmp,&device->config,sizeof(device->config));
    }

    if ( (tmp.c_cflag & PARENB) > 0 )
    {
        return ( (tmp.c_cflag & PARODD) > 0 )?PARITY_ODD:PARITY_EVEN;
    }
    else
    {
        return PARITY_NONE;
    }
}

enum access_mode serial_get_access_mode (SerialDevice device)
{
    int oflag;

    if (device == NULL)
        return ACCESS_NONE;
    if ( device->fd > 0 )
    {
        oflag = fcntl(device->fd, F_GETFL);
        return ( (oflag & O_RDWR) > 0 )?ACCESS_READ_WRITE:
            ((oflag & O_WRONLY) > 0 )?ACCESS_WRITE_ONLY:((oflag & O_RDONLY) > 0 )?
            ACCESS_READ_ONLY:ACCESS_NONE;
    }
    else
    {
        return device->access;
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

int serial_get_file_descriptor (SerialDevice device)
{
    if (device == NULL)
        return -1;
    return device->fd;
}