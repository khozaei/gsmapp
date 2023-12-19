//
// Created by Amin Khozaei on 12/19/23.
//amin.khozaei@gmail.com
//
#include "gsm.h"

#include <stdlib.h>
#include <stdio.h>

static void sendSMS(char *message, char *number);

GSM* gsm_init()
{
    GSM *gsm = calloc(sizeof (GSM), 1);
    if (gsm != NULL)
        gsm->sendSMS = &sendSMS;
    return gsm;
}

void gsm_free(GSM *gsm)
{
    if (gsm != NULL)
        free(gsm);
    gsm = NULL;
}

void sendSMS(char *message, char *number)
{
    printf("SendSMS(%s,%s)\n", message, number);
}