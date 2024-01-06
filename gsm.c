//
// Created by Amin Khozaei on 12/19/23.
//amin.khozaei@gmail.com
//
#include "gsm.h"
#include "serial.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define CMD_MAX_LEN 1024

static GSMDevice gsm_init(const char *port, enum gsm_vendor_model vendor);
static void gsm_free(GSMDevice *gsm);
static void send_sms(GSMDevice device, char *message, char *number);
static void register_sim (GSMDevice device);

static void gsm_init_ai_a7_a6(GSMDevice device);
static void read_serial(int fd, uint8_t *data, size_t len);
static void write_cmd(GSMDevice device, const char *cmd);


struct gsm_device{
    char *port;
    SerialDevice serial;
    enum gsm_vendor_model vendor;
};

const struct _gsm gsm = {
    .init = &gsm_init,
    .free = &gsm_free,
    .send_sms = &send_sms,
    .register_sim = register_sim
};

GSMDevice gsm_init(const char *port, enum gsm_vendor_model vendor)
{
    struct gsm_device *gsm_dev = calloc(sizeof (struct gsm_device), 1);
    if (gsm_dev != NULL) {
        gsm_dev->port = strdup(port);
        gsm_dev->serial = serial.init(port);
        if (gsm_dev->serial == NULL) {
            gsm_free(&gsm_dev);
            return NULL;
        }
        switch (vendor) {
            case GSM_AI_A6:
            case GSM_AI_A7:
                gsm_init_ai_a7_a6(gsm_dev);
                break;
        }
        gsm_dev->vendor = vendor;
        serial.open(gsm_dev->serial);
        serial.enable_async(gsm_dev->serial,read_serial);
    }
    return gsm_dev;
}

void gsm_init_ai_a7_a6(GSMDevice device)
{
    if (device == NULL)
        return;
    serial.set_baudrate(device->serial, 115200);
    serial.set_parity(device->serial, PARITY_NONE);
    serial.set_stopbits(device->serial, 1);
    serial.set_databits(device->serial, 8);
    serial.set_access_mode(device->serial, ACCESS_READ_WRITE);
    serial.set_handshake(device->serial, HANDSHAKE_NONE);
    serial.set_echo(device->serial,false);
}

void gsm_free(GSMDevice *gsm_device)
{
    if ((*gsm_device) != NULL) {
        if ((*gsm_device)->port != NULL)
            free((*gsm_device)->port);
        (*gsm_device)->port = NULL;
        if ((*gsm_device)->serial != NULL){
            serial.close((*gsm_device)->serial);
            serial.free(&((*gsm_device)->serial));
        }
        free((*gsm_device));
    }
    gsm_device = NULL;
}

void send_sms(GSMDevice device, char *message, char *number)
{
    printf("SendSMS(%s,%s) %i\n", message, number,serial.get_file_descriptor(device->serial));
    write_cmd(device, "AT");
}

void read_serial(int fd, uint8_t *data, size_t len)
{
    uint8_t *_data;

    _data = calloc(sizeof (uint8_t), len);
    memcpy(_data, data, len);
    printf("fd=%i,data = [%s], len=%zu\n",fd, _data,len);
    free(_data);
}

void register_sim (GSMDevice device)
{
//    write_cmd(device, "ATE0\r\n", true);
//    uv_sleep(500);
    write_cmd(device, "AT+CREG?");
}

void write_cmd(GSMDevice device, const char *cmd)
{
    printf("cmd= %s, len= %zu\n", cmd, strnlen(cmd, CMD_MAX_LEN));
    serial.write(device->serial,(const uint8_t *)cmd, strnlen(cmd, CMD_MAX_LEN));
    serial.write(device->serial,(const uint8_t *)"\r\n", 2);
}
