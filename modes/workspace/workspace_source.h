#pragma once

#include "files/buffer.h"
#include "brain.h"
#include "workspace/workspace.h"

typedef struct {
    buffer_source header;
    buffer content_buf;
    workspace_list *workspace;
} source_type_workspace;

extern int workspace_type_id;

void brain_workspace_register();

void* brain_create_workspace_source(workspace_list *workspace);

void* brain_create_default_workspace_source();