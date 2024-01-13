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
#include <glib.h>
#include <pthread.h>

#define CMD_MAX_LEN 1024
#define UNUSED(X) (void *)(X)
typedef struct task* Task;

static GSMDevice gsm_init(const char *port, enum gsm_vendor_model vendor);
static void gsm_free(GSMDevice *gsm);
static void send_sms(GSMDevice device, char *message, char *number);
static void register_sim (GSMDevice device);

static gpointer scheduler_init(gpointer data);
static void gsm_init_ai_a7_a6(GSMDevice device);
static void read_serial(int fd, uint8_t *data, size_t len);
static void write_cmd(GSMDevice device, const char *cmd);

static void generic_callback (Task task, GString reply);

struct gsm_device{
    char *port;
    SerialDevice serial;
    enum gsm_vendor_model vendor;
    GMutex mutex;
};

struct task {
    GString *request;
    void (* cb) (Task task, GString reply);
    void (* depend_cb) (Task task, Task last_task, GString reply);
    guint32 timeout; //millisecond
    GString reply;
    bool dependent;
    bool is_reply_ok;
};

GHashTable *task_scheduler;
GHashTable *task_locker;
pthread_t t;

const struct _gsm gsm = {
    .init = &gsm_init,
    .free = &gsm_free,
    .send_sms = &send_sms,
    .register_sim = register_sim
};

void hash_key_destroy (gpointer value)
{
    if (value != NULL)
        g_free(value);
}

void hash_value_destroy (gpointer value)
{
    if (value != NULL)
        g_free(value);
}

void * scheduler_task ()
{
    while (true) {
        printf("scheduler_task\n");
        g_usleep(1000* 500);
    }
    return NULL;
}

gpointer scheduler_init(gpointer data)
{
    UNUSED(data);
    printf("scheduler_init\n");
    task_scheduler = g_hash_table_new_full(g_int_hash,
                                           g_int_equal,
                                           (GDestroyNotify) hash_key_destroy,
                                           (GDestroyNotify) hash_value_destroy);
    task_locker = g_hash_table_new_full(g_int_hash,
                                        g_int_equal,
                                        (GDestroyNotify) hash_key_destroy,
                                        NULL);
    pthread_create(&t,NULL,scheduler_task,NULL);
    return NULL;
}

GSMDevice gsm_init(const char *port, enum gsm_vendor_model vendor)
{
    struct gsm_device *gsm_dev = calloc(sizeof (struct gsm_device), 1);
    if (gsm_dev != NULL) {
        gint *fd;

        fd = g_new(gint, 1);
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
        static GOnce once = G_ONCE_INIT;
        g_once(&once, scheduler_init, NULL);
        g_mutex_init(&gsm_dev->mutex);
        *fd = serial.get_file_descriptor(gsm_dev->serial);
        if (g_hash_table_lookup(task_locker,fd) == NULL)
            g_hash_table_insert(task_locker, fd, &gsm_dev->mutex);
        else
            g_free(fd);
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
    struct task *task;
    GQueue *tasks;
    gint *fd;

    g_assert(device != NULL);
    if (device == NULL)
        return;
    printf("SendSMS(%s,%s) %i\n", message, number,serial.get_file_descriptor(device->serial));
    task = malloc(sizeof (struct task));
    if (task == NULL)
        return;
    task->request = g_string_new("AT");
    task->timeout = 100;
    task->dependent = false;
    fd = g_new(gint,1);
    *fd = serial.get_file_descriptor(device->serial);
    tasks = g_hash_table_lookup(task_scheduler,fd);
    if (tasks == NULL) {
        tasks = g_queue_new();
        g_hash_table_insert(task_scheduler, fd, tasks);
    } else
        g_free(fd);
    g_queue_push_head(tasks,task);
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

void generic_callback (Task task, GString reply)
{
    GString local_reply;

    g_string_assign(&local_reply,reply.str);
    g_string_ascii_up(&local_reply);
    if (g_strstr_len(local_reply.str,(gssize)local_reply.len,"OK") != NULL)
        if (task != NULL)
            task->is_reply_ok = true;
}