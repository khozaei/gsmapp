//
// Created by amin on 1/30/24.
//

#include "smartpointer.h"

#include <stdlib.h>

static unique_ptr_t unique_ptr_make (void *ptr, free_func_ptr free);
static unique_ptr_t unique_ptr_move (unique_ptr_t *src);
static void *unique_ptr_get (unique_ptr_t ptr);
static shared_ptr_t shared_ptr_make (void *ptr, free_func_ptr free);
static shared_ptr_t shared_ptr_copy (shared_ptr_t ptr);
static shared_ptr_t shared_ptr_move (shared_ptr_t src);
static void *shared_ptr_get (shared_ptr_t ptr);
static int get_ref_count (shared_ptr_t ptr);



struct unique_ptr_s{
    void *ptr;
    free_func_ptr free;
};

struct shared_ptr_s {
    void *ptr;
    free_func_ptr free;
    int *ref_count;
};

const struct smartpointer_s smartpointer = {
        .unique_ptr_make = &unique_ptr_make,
        .unique_ptr_move = unique_ptr_move,
        .unique_ptr_get = unique_ptr_get,
        .shared_ptr_make = shared_ptr_make,
        .shared_ptr_copy = shared_ptr_copy,
        .shared_ptr_move = shared_ptr_move,
        .shared_ptr_get = shared_ptr_get,
        .get_ref_count = get_ref_count,
};

unique_ptr_t unique_ptr_make (void *ptr, free_func_ptr free)
{
    unique_ptr_t new_ptr;

    if (ptr == NULL) return NULL;
    new_ptr = malloc(sizeof(struct unique_ptr_s));
    if (new_ptr == NULL) return NULL;

    new_ptr->ptr = ptr;
    new_ptr->free = free;
    return new_ptr;
}

unique_ptr_t unique_ptr_move (unique_ptr_t *src)
{
    unique_ptr_t target;

    if (src == NULL) return NULL;
    if (*src == NULL) return NULL;

    target = malloc(sizeof(unique_ptr_t));
    if (target == NULL) return NULL;

    target->ptr = (*src)->ptr;
    target->free = (*src)->free;

    free(*src);
    *src = NULL;

    return target;
}

void *unique_ptr_get (unique_ptr_t ptr)
{
    if (ptr == NULL) return NULL;
    return ptr->ptr;
}

shared_ptr_t shared_ptr_make (void *ptr, free_func_ptr free)
{
    __attribute__((cleanup(shared_ptr_destroy))) shared_ptr_t new_ptr;

    new_ptr = malloc(sizeof(shared_ptr_t));
    if (new_ptr == NULL) return NULL;

    new_ptr->ptr = ptr;
    new_ptr->free = free;
    new_ptr->ref_count = malloc(sizeof(int));
    *new_ptr->ref_count = 1;
    return new_ptr;
}

shared_ptr_t shared_ptr_copy (shared_ptr_t ptr)
{
    shared_ptr_t copy;
    if (ptr == NULL) return NULL;

    copy = malloc(sizeof(shared_ptr_t));
    if (copy == NULL) return NULL;

    copy->ptr = ptr->ptr;
    copy->free = ptr->free;
    copy->ref_count = ptr->ref_count;
    ptr->ref_count++;

    return copy;
}

shared_ptr_t shared_ptr_move (shared_ptr_t src)
{
    shared_ptr_t target;
    if (src == NULL) return NULL;

    target = malloc(sizeof(shared_ptr_t));
    if (target == NULL) return NULL;

    target->ptr = src->ptr;
    target->free = src->free;
    target->ref_count = src->ref_count;

    free(src);
    return target;
}

void *shared_ptr_get (shared_ptr_t ptr)
{
    if (ptr == NULL) return NULL;
    return ptr->ptr;
}

int get_ref_count (shared_ptr_t ptr)
{
    if (ptr == NULL) return -1;
    return *ptr->ref_count;
}

void unique_ptr_destroy(unique_ptr_t *ptr)
{
    if (ptr == NULL) return;
    if (*ptr == NULL) return;

    if ((*ptr)->free != NULL) {
        (*ptr)->free((*ptr)->ptr);
    }

    free(*ptr);
    *ptr = NULL;
}

void shared_ptr_destroy(shared_ptr_t *ptr)
{
    if (ptr == NULL) return;
    if (*ptr == NULL) return;

    (*ptr)->ref_count--;

    if (get_ref_count(*ptr) <= 0) {
        if ((*ptr)->free != NULL) {
            ((*ptr)->free)((*ptr)->ptr);
        }
        free((*ptr)->ref_count);
        free(*ptr);
        *ptr = NULL;
    }
}