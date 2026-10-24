#pragma once

#include "files/buffer.h"
#include "brain.h"
#include "workspace/workspace.h"

typedef struct {
    buffer_source header;
    buffer content_buf;
    workspace_list *workspace;
    bool show_dotfiles;
    arr_stack_t *format_stack;
    linked_list_t *expanded_dirs;
} source_type_workspace;

extern int workspace_type_id;

void brain_workspace_register();

void* brain_create_workspace_source(workspace_list *workspace);

void* brain_create_default_workspace_source();

bool brain_workspace_source_handle_kbd(buffer_source *ctx, kbd_event ev, u8 modifier);

bool brain_workspace_source_handle_mouse(buffer_source *ctx, mouse_data data, u8 modifier);