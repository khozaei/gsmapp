    //
// Created by amin on 12/19/23.
//

#ifndef GSMAPP_SERIAL_H
#define GSMAPP_SERIAL_H

struct serial {

};

typedef struct serial Serial;

Serial *serial_init(char *serialFile);
void serial_free(Serial *serial);

#endif //GSMAPP_SERIAL_H
