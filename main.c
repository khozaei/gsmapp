#include "gsm.h"

#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    GSMDevice gsm_device = gsm.init();
    gsm.sendSMS("gholi", "09214528198");
    gsm.free(&gsm_device);
    return 0;
}
