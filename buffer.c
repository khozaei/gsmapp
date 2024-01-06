//
// Created by amin on 1/5/24.
//

#include "buffer.h"

#include <glib.h>
#include <stdbool.h>
#include <pthread.h>

static Buffer buffer_init (size_t size);
static void buffer_free(Buffer *_buffer);
static void buffer_push (Buffer _buffer, const char *str);
static void buffer_pop_len (Buffer _buffer, char *str, size_t len);
static void buffer_pop_break (Buffer _buffer, char *str);

struct _t_buffer {
    GString *content;
    gsize   size;
    pthread_mutex_t mutex;
};

const struct _buffer buffer = {
    .init = &buffer_init,
    .free = &buffer_free,
    .push = &buffer_push,
    .pop_len = &buffer_pop_len,
    .pop_break = &buffer_pop_break,
};

Buffer buffer_init (size_t size)
{
    struct _t_buffer *_buffer = calloc(sizeof (struct _t_buffer), 1);
    g_assert(_buffer != NULL);

    if (_buffer != NULL) {
        int mutex_res;

        _buffer->size = size;
        _buffer->content = g_string_sized_new(size);
        mutex_res = pthread_mutex_init(&_buffer->mutex, NULL);
        g_assert(mutex_res == 0);
    }
    return _buffer;
}

void buffer_free(Buffer *_buffer)
{
    if ((*_buffer) == NULL)
        return;

    if ((*_buffer)->content != NULL) {
        g_string_free((*_buffer)->content, true);
        (*_buffer)->content = NULL;
        (*_buffer)->size = 0;
    }
    free((*_buffer));
    _buffer = NULL;
}

void buffer_push (Buffer _buffer, const char *str)
{
    g_assert(_buffer != NULL);
    if (_buffer == NULL)
        return;

    pthread_mutex_lock(&_buffer->mutex);
    g_string_append(_buffer->content, str);
    pthread_mutex_unlock(&_buffer->mutex);
}


void buffer_pop_len (Buffer _buffer, char *str, size_t len)
{
    gchar *local_str;

    g_assert(_buffer != NULL);
    g_assert(str != NULL);
    g_assert(len > 0);
    if (_buffer == NULL)
        return;

    if (str == NULL)
        return;

    if (len <= 0)
        return;

    pthread_mutex_lock(&_buffer->mutex);
    local_str = g_utf8_substring(_buffer->content->str, 0, (glong)len);
    g_strlcpy(str,local_str,len);
    g_string_replace(_buffer->content, local_str, "", 1);
    g_free(local_str);
    pthread_mutex_unlock(&_buffer->mutex);
}

void buffer_pop_break (Buffer _buffer, char *str)
{
    bool local_break;
    guint32 local_index;

    local_break = false;
    local_index = 0;
    g_assert(_buffer != NULL);
    g_assert(str != NULL);
    if (_buffer == NULL)
        return;

    if (str == NULL)
        return;

    pthread_mutex_lock(&_buffer->mutex);
    for (int i = 0; i < _buffer->content->len; i++) {
        if (_buffer->content->str[i] == '\r' || _buffer->content->str[i] == '\n' )
            local_break = true;
        else if (local_break)
            break;
        local_index++;
    }
    pthread_mutex_unlock(&_buffer->mutex);
    if (local_index > 0)
        buffer_pop_len(_buffer, str, local_index);
}