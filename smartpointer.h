//
// Created by amin on 1/30/24.
//

#ifndef GSMAPP_SMARTPOINTER_H
#define GSMAPP_SMARTPOINTER_H

#define unique_ptr __attribute__((cleanup(unique_ptr_destroy))) unique_ptr_t
#define shared_ptr __attribute__((cleanup(shared_ptr_destroy))) shared_ptr_t

typedef void (*free_func_ptr) (void *);
typedef struct unique_ptr_s * unique_ptr_t;
typedef struct shared_ptr_s * shared_ptr_t;

struct smartpointer_s {
    unique_ptr_t (*unique_ptr_make) (void *ptr, free_func_ptr free);
    unique_ptr_t (*unique_ptr_move) (unique_ptr_t *src);
    void *(*unique_ptr_get) (unique_ptr_t ptr);

    shared_ptr_t (*shared_ptr_make) (void *ptr, free_func_ptr free);
    shared_ptr_t (*shared_ptr_copy) (shared_ptr_t ptr);
    shared_ptr_t (*shared_ptr_move) (shared_ptr_t src);
    void *(*shared_ptr_get) (shared_ptr_t ptr);
    int (*get_ref_count) (shared_ptr_t ptr);
};

extern const struct smartpointer_s smartpointer;
void unique_ptr_destroy(unique_ptr_t *ptr);
void shared_ptr_destroy(shared_ptr_t *ptr);
#endif //GSMAPP_SMARTPOINTER_H
