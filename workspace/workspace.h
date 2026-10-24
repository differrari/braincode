#pragma once

#include "types.h"
#include "string/slice.h"
#include "data/struct/linked_list.h"

typedef linked_list_t workspace_list;

typedef struct {
    string s;
} workspace_entry;

workspace_list* default_workspace();

bool add_to_workspace(workspace_list *workspace, string_slice s);
bool remove_from_workspace(workspace_list *workspace, string_slice s);