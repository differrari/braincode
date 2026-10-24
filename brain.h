#pragma once

#include "uno.h"

typedef struct {
    int type;
    text_field_info text_info;
    void (*command_handler)(void *ctx, string_slice cmd);
    uptr cursors[1];
    text_format_arr formatting;
} buffer_source;

int register_source_type();
void current_buffer_open(char *str);