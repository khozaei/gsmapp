#include "gsm.h"

#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    GSM* gsm = gsm_init();
    gsm->sendSMS("gholi", "09214528198");
    gsm_free(gsm);
    return 0;
}
