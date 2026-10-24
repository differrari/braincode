#pragma once

#include "files/buffer.h"
#include "brain.h"
#include "shell/shell.h"

typedef struct {
    buffer_source header;
    shell_handle *shell;
} source_type_shell;

extern int shell_type_id;

void brain_shell_register();

void* brain_create_shell_source(void (*builtin)(shell_handle*), bool script_only, bool ignore_print);

void* brain_create_default_shell_source();

bool brain_shell_source_handle_kbd(buffer_source *ctx, kbd_event ev, u8 modifier);

bool brain_shell_source_handle_mouse(buffer_source *ctx, mouse_data data, u8 modifier);