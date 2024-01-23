#include "gsm.h"

#include "uv.h"

int main() {
    uv_tcp_t server;
    struct sockaddr_in bind_addr;

    GSMDevice gsm_device = gsm.init("/dev/ttyUSB0", GSM_AI_A7);

    gsm.send_sms(gsm_device,"gholi", "09214528198");
    uv_sleep(200);
    while (1) {
//        buffer.push(ring_buffer, "GHOLI \r\n \r   i \n");
//        gsm.send_sms(gsm_device,"aa","09214528198");
        gsm.register_sim(gsm_device);
        uv_sleep(400);
    }


    uv_tcp_init(uv_default_loop(), &server);
    uv_ip4_addr("0.0.0.0", 2986, &bind_addr);
    uv_tcp_bind(&server, (const struct sockaddr *)&bind_addr, 0);
    uv_listen((uv_stream_t *)&server, 128, NULL);
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());

//    gsm.free(&gsm_device);
    return 0;
}
