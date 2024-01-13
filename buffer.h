//
// Created by amin on 1/5/24.
//

#ifndef GSMAPP_BUFFER_H
#define GSMAPP_BUFFER_H

#include <stddef.h>

typedef struct _t_buffer *Buffer;

struct _buffer {
    Buffer      (* init) (size_t size);
    void        (* free) (Buffer *buffer);

    void        (* push) (Buffer ring, const char *str);
    void        (* pop_len) (Buffer ring, char *str, size_t len);
    void        (* pop_break) (Buffer ring, char *str);
};
extern const struct _buffer buffer;

#endif //GSMAPP_BUFFER_H
