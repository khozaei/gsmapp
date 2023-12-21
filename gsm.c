//
// Created by Amin Khozaei on 12/19/23.
//amin.khozaei@gmail.com
//
#include "gsm.h"

#include <stdlib.h>
#include <stdio.h>

static GSMDevice gsm_init();
static void gsm_free(GSMDevice *gsm);
static void sendSMS(char *message, char *number);


struct _gsm_device{


};

const struct _gsm gsm = {
    .init = &gsm_init,
    .free = &gsm_free,
    .sendSMS = &sendSMS
};

GSMDevice gsm_init()
{
    struct _gsm_device *gsm_dev = calloc(sizeof (struct _gsm_device), 1);
    return gsm_dev;
}

void gsm_free(GSMDevice *gsm_device)
{
    if ((*gsm_device) != NULL)
        free((*gsm_device));
    gsm_device = NULL;
}

void sendSMS(char *message, char *number)
{
    printf("SendSMS(%s,%s)\n", message, number);
}