#include "workspace.h"
#include "alloc/alloc.h"

workspace_list *workspace_contents;

workspace_list* default_workspace(){
    if (!workspace_contents) workspace_contents = linked_list_create();
    return workspace_contents;
}

static int list_contains(workspace_list *workspace, string_slice s){
    if (!workspace) return -1;
    int count = 0;
    for (linked_list_node_t *node = workspace->head; node; node = node->next){
        workspace_entry *entry = node->data;
        if (entry && slices_equal(s, slice_from_string(entry->s), true)) return count;
        count++;
    }
    return -1;
}

bool add_to_workspace(workspace_list *workspace, string_slice s){
    if (!workspace) return false;
    if (list_contains(workspace,s) >= 0) return false;
    workspace_entry *entry = new(workspace_entry);
    entry->s = string_from_literal_length(s.data, s.length);
    linked_list_push(workspace, entry);
    print("Added %S to workspace",s);
    return true;
}

bool remove_from_workspace(workspace_list *workspace, string_slice s){
    if (!workspace) return false;
    int index = list_contains(workspace,s);
    if (index < 0) return false;
    linked_list_node_t *node = linked_list_get_node(workspace, index);
    if (!node || !node->data) return false;
    workspace_entry *entry = node->data;
    string_free(entry->s);
    linked_list_remove(workspace,node);
    print("Removed %S from workspace",s);
    return true;
}