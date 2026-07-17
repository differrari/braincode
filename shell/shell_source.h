#pragma once

#include "files/buffer.h"
#include "brain.h"
#include "shell/shell.h"

typedef struct {
    buffer_source header;
    buffer buf;
    shell_handle *shell;
} source_type_shell;

extern int shell_type_id;

void* brain_create_shell_source();