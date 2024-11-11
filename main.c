#include "gsm.h"
#include "smartpointer.h"
#include "version.h"

#include "uv.h"

#include <stdlib.h>
#include <string.h>

struct testsmart{
    char *name;
    int value;
};

void free_test(void *ptr){
    struct testsmart *t;

    t = (struct testsmart *)ptr;
    if (t == NULL) return;
    printf("destroy: %s,%i\n",t->name, t->value);
    if (t->name != NULL)
        free(t->name);
    free(t);
}

void test_scope() {
    struct testsmart *t;

    t = malloc(sizeof (struct testsmart));
    if (t == NULL) return;
    t->value = 76;
    t->name = malloc(sizeof (char) * 10);

    if (t->name)
        sprintf(t->name, "Gholi");
    printf("testsmart: %s,%i\n", t->name,t->value);
    unique_ptr ptr = smartpointer.unique_ptr_make(t, free_test);
    if (ptr != NULL) {
        printf("unique: %s\n", ((struct testsmart *)smartpointer.unique_ptr_get(ptr))->name);
    }
}

void print_version() {
    printf("Application Version: %s\n", PROJECT_VERSION);
    printf("Version Details:\n");
    printf("Major: %d\n", PROJECT_VERSION_MAJOR);
    printf("Minor: %d\n", PROJECT_VERSION_MINOR);
    printf("Patch: %d\n", PROJECT_VERSION_PATCH);
    printf("Build Date: %s\n", __DATE__);
    printf("Build Time: %s\n", __TIME__);
}

int main(int argc, char *argv[]) {
    uv_tcp_t server;
    struct sockaddr_in bind_addr;

    for (int i = 1; i < argc; i++) {
        // Version checks
        if (strcmp(argv[i], "-v") == 0 || 
            strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        }
    }
    test_scope();

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
