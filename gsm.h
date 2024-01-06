//
// Created by Amin Khozaei on 12/19/23.
// amin.khozaei@gmail.com
//

#ifndef GSMAPP_GSM_H
#define GSMAPP_GSM_H

typedef struct gsm_device *GSMDevice;

enum gsm_vendor_model {
    GSM_AI_A7 = 0x0100,
    GSM_AI_A6
};

struct _gsm{
    GSMDevice   (* init) (const char *port, enum gsm_vendor_model vendor);
    void        (* free) (GSMDevice *device);

    void (*register_sim) (GSMDevice device);
    void (*send_sms) (GSMDevice device,char *message, char *number);
};
extern const struct _gsm gsm;



//GSM* gsm_init();
//void gsm_free(GSM *gsm);
#endif //GSMAPP_GSM_H
