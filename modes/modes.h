#pragma once

typedef void* (*mode_init_fn)();

int register_mode_type(mode_init_fn fn);

void* create_mode_buffer(int mode);
void* create_mode_buffer_default();

void mode_set_default(int mode);