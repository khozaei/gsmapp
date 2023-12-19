//
// Created by Amin Khozaei on 12/19/23.
// amin.khozaei@gmail.com
//

#ifndef GSMAPP_GSM_H
#define GSMAPP_GSM_H

struct gsm {
    //void (*register_sim)();
    void (*sendSMS)(char *message, char *number);
};

typedef struct gsm GSM;

GSM* gsm_init();
void gsm_free(GSM *gsm);
#endif //GSMAPP_GSM_H
