#include "gsm.h"
#include "serial.h"

#include "uv.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

void read_serial(int fd, uint8_t *data, size_t len)
{
    uint8_t *_data;

    _data = calloc(sizeof (uint8_t), len);
    memcpy(_data, data, len);
    printf("fd=%i,data = [%s], len=%zu\n",fd, _data,len);
}

int main() {
    uv_tcp_t server;
    struct sockaddr_in bind_addr;

    printf("Hello, World!\n");
    GSMDevice gsm_device = gsm.init();
    gsm.sendSMS("gholi", "09214528198");
    gsm.free(&gsm_device);
    SerialDevice dev = serial.init("/dev/ttyUSB2");
    serial.set_baudrate(dev, 115200);
    serial.set_parity(dev, PARITY_NONE);
    serial.set_stopbits(dev, 1);
    serial.set_databits(dev, 8);
    serial.set_access_mode(dev, ACCESS_READ_WRITE);
    serial.set_handshake(dev, HANDSHAKE_NONE);
    serial.set_echo(dev,false);
    serial.open(dev);
    serial.enable_async(dev,read_serial);
    serial.write(dev, (uint8_t *)"AT\r\n", 5);
//    uint8_t data[64];
    sleep(1);
    serial.write(dev, (uint8_t *)"AT\r\n", 5);
//    serial.read(dev, data, 63, 500);
//    printf("read from serial: %s\n",data);

    uv_tcp_init(uv_default_loop(), &server);
    uv_ip4_addr("0.0.0.0", 2986, &bind_addr);
    uv_tcp_bind(&server, (const struct sockaddr *)&bind_addr, 0);
    uv_listen((uv_stream_t *)&server, 128, NULL);
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());
    serial.close(dev);
    return 0;
}
