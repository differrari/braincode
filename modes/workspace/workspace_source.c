#include "workspace_source.h"
#include "modes/modes.h"
#include "utils/indent.h"
#include "files/helpers.h"

int workspace_type_id = 0;

void brain_workspace_register(){
    workspace_type_id = register_mode_type(brain_create_default_workspace_source);
}

source_type_workspace *new_workspace_ptr;
int indentation = 0;

void workspace_source_traverse(const char *directory, const char *file){
    print("%s",file);
    buffer_write(&new_workspace_ptr->content_buf, "%s%s\n",indent_by(indentation),file);
}

void workspace_source_list(void *data){
    if (!new_workspace_ptr) return;
    print("Valid workspace");
    workspace_entry *entry = data;
    if (!entry || !entry->s.length) return;
    print("Checking entry %S",entry->s);
    buffer_write(&new_workspace_ptr->content_buf, "%s%S\n",indent_by(indentation),entry->s);
    indentation++;
    traverse_directory(entry->s.data, false, workspace_source_traverse);
    indentation--;
}

void* brain_create_workspace_source(workspace_list *workspace){
    if (!workspace_type_id) brain_workspace_register();
    source_type_workspace *new_workspace = zalloc(sizeof(source_type_workspace));
    new_workspace->header.type = workspace_type_id;
    new_workspace->workspace = workspace;
    new_workspace->content_buf = buffer_create(0x100, buffer_can_grow);
    new_workspace->header.text_info = (text_field_info){
        .content = &new_workspace->content_buf,
        .placeholder = SLICE("EMPTY WORKSPACE"),
    };
    new_workspace_ptr = new_workspace;
    linked_list_for_each(workspace, workspace_source_list);
    new_workspace_ptr = 0;
    return new_workspace;
}

void* brain_create_default_workspace_source(){
    return brain_create_workspace_source(default_workspace());
}