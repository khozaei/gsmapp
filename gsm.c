//
// Created by Amin Khozaei on 12/19/23.
//amin.khozaei@gmail.com
//
#include "gsm.h"
#include "serial.h"
#include "buffer.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <glib.h>
#include <pthread.h>

#define CMD_MAX_LEN 1024
#define REPLY_MAX_LEN 4096
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

static void generic_process (Task task);

struct gsm_device{
    char *port;
    SerialDevice serial;
    enum gsm_vendor_model vendor;

    GMutex mutex;
    pthread_t thread;
    gint *fd;
    Buffer buffer;
};

struct task {
    GString *request;
    void (* cb) (Task task);
    guint32 timeout; //millisecond
    GString *reply;
    Task priv_task;
    bool is_reply_ok;
    bool is_sent;
};

GHashTable *task_scheduler;
GHashTable *task_devices;
GMutex mutex_scheduler;
GMutex mutex_devices;

const struct _gsm gsm = {
    .init = &gsm_init,
    .free = &gsm_free,
    .send_sms = &send_sms,
    .register_sim = register_sim
};

void task_free (Task *task)
{
    if (task == NULL)
        return;
    if ((*task) == NULL)
        return;
    if ((*task)->request != NULL)
        g_free((*task)->request);
    if ((*task)->reply != NULL)
        g_free((*task)->reply);
    if ((*task)->priv_task != NULL)
        task_free(&(*task)->priv_task);
    free(task);
    task = NULL;
}

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

void hash_device_destroy (gpointer value)
{
    //TODO
    //gsm_free(&((GSMDevice)value));
}

void *buffer_process (void *device_pointer)
{
    GSMDevice device = (GSMDevice)device_pointer;
    GQueue  *tasks;
    Task    task;
    char    buf[REPLY_MAX_LEN];

    if (device == NULL)
        return NULL;
    if (device->buffer == NULL)
        return NULL;
    while (true){
        memset(buf,0,REPLY_MAX_LEN);
        buffer.pop_break(device->buffer, buf);
        if (strnlen(buf,REPLY_MAX_LEN) == 0){
            g_usleep(1000 * 1);
            continue;
        }
        g_mutex_lock(&mutex_scheduler);
        tasks = (GQueue *)g_hash_table_lookup(task_scheduler, device->fd);
        g_mutex_unlock(&mutex_scheduler);
        if (tasks == NULL)
            continue;
        g_mutex_lock(&device->mutex);
        task = (Task)g_queue_peek_head(tasks);
        if (task == NULL) {
            g_mutex_unlock(&device->mutex);
            g_usleep(1000 * 10);
            continue;
        }
        if (!task->is_sent){
            g_mutex_unlock(&device->mutex);
            g_usleep(1000 * 10);
            continue;
        }
        if (task->reply == NULL)
            task->reply = g_string_new(buf);
        else
            g_string_append(task->reply,buf);
        g_mutex_unlock(&device->mutex);
        g_string_ascii_up(task->reply);
        task->is_reply_ok = (g_strstr_len(task->reply->str,
                                          (gssize)task->reply->len,"OK") != NULL);
        if (task->cb != NULL)
            task->cb(task);

    }
    return NULL;
}

void *scheduler_task (void *device_pointer)
{
    GSMDevice device = (GSMDevice)device_pointer;
    GQueue  *tasks;
    Task    task;

    if (device == NULL)
        return NULL;
    while (true) {
        g_mutex_lock(&mutex_scheduler);
        tasks = (GQueue *)g_hash_table_lookup(task_scheduler, device->fd);
        g_mutex_unlock(&mutex_scheduler);
        if (tasks == NULL)
            continue;
        g_mutex_lock(&device->mutex);
        task = (Task)g_queue_peek_head(tasks);
        if (task == NULL) {
            g_mutex_unlock(&device->mutex);
            g_usleep(1000 * 10);
            continue;
        }
        if (task->is_sent) {
            //remove sent task after timeout
            Task tmp;

            tmp = (Task)g_queue_pop_head(tasks);
            if (tmp != NULL) {
                task_free(&tmp);
                tmp = (Task)g_queue_peek_head(tasks);
                if (tmp != NULL) {
                    tmp->priv_task = NULL;
                    tmp->is_sent = true;//to remove in next run of the loop
                }
            }
            g_mutex_unlock(&device->mutex);
            continue;
        }
        if (task->priv_task != NULL) {
            if (task->priv_task->is_reply_ok)
                write_cmd(device, task->request->str);
            else
                task->is_reply_ok = false;
        }
        if (!task->is_sent)
            write_cmd(device, task->request->str);
        printf("scheduler_task\n");
        g_mutex_unlock(&device->mutex);
        g_usleep(1000 * task->timeout);
    }
    return NULL;
}

gpointer scheduler_init(gpointer data)
{
    UNUSED(data);
    printf("scheduler_init\n");
    task_scheduler = g_hash_table_new_full (g_int_hash,
                                           g_int_equal,
                                           (GDestroyNotify) hash_key_destroy,
                                           (GDestroyNotify) hash_value_destroy);
    task_devices = g_hash_table_new_full(g_int_hash,
                                         g_int_equal,
                                         (GDestroyNotify)hash_key_destroy,
                                         (GDestroyNotify) hash_device_destroy);
    g_mutex_init(&mutex_scheduler);
    g_mutex_init(&mutex_devices);
    return NULL;
}

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
        static GOnce once = G_ONCE_INIT;
        g_once(&once, scheduler_init, NULL);
        g_mutex_init(&gsm_dev->mutex);
        gsm_dev->fd = g_new(gint,1);
        if (gsm_dev->fd == NULL) {
            gsm_free(&gsm_dev);
            exit(EXIT_FAILURE);
        }
        *gsm_dev->fd = serial.get_file_descriptor (gsm_dev->serial);
        if (task_devices != NULL) {
            g_mutex_lock(&mutex_devices);
            g_hash_table_insert(task_devices, gsm_dev->fd,gsm_dev);
            g_mutex_unlock(&mutex_devices);
        }
        gsm_dev->buffer = buffer.init(REPLY_MAX_LEN);
        pthread_create(&gsm_dev->thread,NULL,scheduler_task,gsm_dev);
        pthread_create(&gsm_dev->thread,NULL,buffer_process,gsm_dev);
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

    g_assert(device != NULL);
    if (device == NULL)
        return;
    printf("SendSMS(%s,%s) %i\n", message, number,*device->fd);
    task = malloc(sizeof (struct task));
    g_assert(task !=NULL);
    if (task == NULL)
        return;
    task->request = g_string_new("AT");
    task->timeout = 100;
    task->cb = generic_process;
    g_mutex_lock(&mutex_scheduler);
    tasks = g_hash_table_lookup(task_scheduler,device->fd);
    g_mutex_unlock(&mutex_scheduler);
    g_mutex_lock(&device->mutex);
    if (tasks == NULL) {
        tasks = g_queue_new();
        g_assert(tasks != NULL);
        g_mutex_lock(&mutex_scheduler);
        g_hash_table_insert(task_scheduler, device->fd, tasks);
        g_mutex_unlock(&mutex_scheduler);
    }
    g_queue_push_tail(tasks,task);
    g_mutex_unlock(&device->mutex);
}

void read_serial(int fd, uint8_t *data, size_t len)
{
    GSMDevice device;
    gint *local_fd;

    printf("read_serial:[%s]\n",data);
    g_assert(task_devices != NULL);

    if (!task_devices)
        return;
    local_fd = g_new(gint,1);
    *local_fd = fd;
    g_mutex_lock(&mutex_devices);
    device = g_hash_table_lookup(task_devices, local_fd);
    g_mutex_unlock(&mutex_devices);
    g_free(local_fd);
    if (device == NULL)
        return;
    if (device->buffer == NULL)
        return;
    if (strnlen((const char *)data,len) == len)
        data[len - 1] = '\0';//insure null terminating string
    printf("read_serial\n");
    buffer.push(device->buffer,(const char *)data);
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

void generic_process (Task task)
{
    if (task == NULL)
        return;
    if (task->reply == NULL)
        return;
    printf("generic_process\n");
}