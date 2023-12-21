//
// Created by Amin Khozaei on 12/19/23.
// amin.khozaei@gmail.com
//

#ifndef GSMAPP_GSM_H
#define GSMAPP_GSM_H

typedef struct _gsm_device *GSMDevice;

struct _gsm{
    GSMDevice   (* init) ();
    void        (* free) (GSMDevice *device);
    //void (*register_sim)();
    void (*sendSMS)(char *message, char *number);
};
extern const struct _gsm gsm;



//GSM* gsm_init();
//void gsm_free(GSM *gsm);
#endif //GSMAPP_GSM_H
