#include "modes.h"
#include "data/struct/chunk_array.h"
#include "syscalls/syscalls.h"

int default_mode = -1;

chunk_array_t *modes;

int register_mode_type(mode_init_fn fn){
    if (!modes) modes = chunk_array_create(16, sizeof(mode_init_fn));
    int val = chunk_array_push(modes, &fn);
    if (default_mode < 0) default_mode = val;
    return val;
}

void* create_mode_buffer(int mode){
    if (!modes) return 0;
    if (chunk_array_count(modes) <= mode) return 0;
    return (CHUNK_ARRAY_GET(mode_init_fn,modes, mode))();
}

void* create_mode_buffer_default(){
    return create_mode_buffer(default_mode);
}

void mode_set_default(int mode){
    if (!modes) return;
    if (chunk_array_count(modes) <= mode) return;
    default_mode = mode;
}