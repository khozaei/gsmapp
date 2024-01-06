#include "gsm.h"
#include "buffer.h"

#include "uv.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

Buffer ring_buffer;

void * consumer()
{
    char str[100];

    while (1) {
        memset(str,0,100);
        buffer.pop_break(ring_buffer, str);
        printf("consumer: %s\n", str);
        uv_sleep(350);
    }
}

int main() {
    uv_tcp_t server;
    struct sockaddr_in bind_addr;
    pthread_t t;

    ring_buffer = buffer.init(2024);
    pthread_create(&t,NULL,consumer,NULL);
    printf("Hello, World!\n");
    GSMDevice gsm_device = gsm.init("/dev/ttyUSB0", GSM_AI_A7);
    gsm.send_sms(gsm_device,"gholi", "09214528198");
    uv_sleep(200);
    while (1) {
        buffer.push(ring_buffer, "GHOLI \r\n \r   i \n");
//        gsm.register_sim(gsm_device);
        uv_sleep(400);
    }
    buffer.free(&ring_buffer);


    uv_tcp_init(uv_default_loop(), &server);
    uv_ip4_addr("0.0.0.0", 2986, &bind_addr);
    uv_tcp_bind(&server, (const struct sockaddr *)&bind_addr, 0);
    uv_listen((uv_stream_t *)&server, 128, NULL);
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());

    gsm.free(&gsm_device);
    return 0;
}
