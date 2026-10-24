#include "workspace_source.h"
#include "modes/modes.h"
#include "utils/indent.h"
#include "files/helpers.h"
#include "kbd_helper.h"
#include "input_keycodes.h"
#include "data/struct/linked_list.h"

int workspace_type_id = 0;

void brain_workspace_register(){
    workspace_type_id = register_mode_type(brain_create_default_workspace_source);
}

source_type_workspace *new_workspace_ptr;
int indentation = 0;

linked_list_node_t* workspace_expands_dir(source_type_workspace *workspace, string_slice s){
    for (linked_list_node_t *node = workspace->expanded_dirs->head; node; node = node->next){
        if (!strncmp(node->data, s.data, s.length)) return node;
    }
    return 0;
}

void workspace_source_traverse(const char *directory, const char *file){
    size_t len = strlen(file);
    if (!new_workspace_ptr->show_dotfiles && len && *file == '.') return;
    buffer_write(&new_workspace_ptr->content_buf, "%s",indent_by(indentation));
    string s = string_format("%s/%s",directory,file);
    range_t r = { .start = new_workspace_ptr->content_buf.cursor, .size = len };
    text_format fmt = {
        .bounds = r,
        .link = s,
    };
    fs_stat out_stat = {};
    if (statf(s.data, &out_stat)){
        fmt.foreground = out_stat.type == entry_directory ? 0xff3232cc : 0xffcccccc;
    }
    stack_push(new_workspace_ptr->format_stack, &fmt);
    buffer_write(&new_workspace_ptr->content_buf, "%s\n",file);
    if (workspace_expands_dir(new_workspace_ptr, slice_from_string(s))){
        indentation++;
        traverse_directory(s.data, false, workspace_source_traverse);
        indentation--;
    }
}

void workspace_source_list(void *data){
    if (!new_workspace_ptr) return;
    workspace_entry *entry = data;
    if (!entry || !entry->s.length) return;
    buffer_write(&new_workspace_ptr->content_buf, "%s%S\n",indent_by(indentation),entry->s);
    indentation++;
    traverse_directory(entry->s.data, false, workspace_source_traverse);
    indentation--;
}

void brain_workspace_refresh(source_type_workspace *workspace){
    new_workspace_ptr = workspace;
    buffer_wipe(&workspace->content_buf);
    if (!workspace->format_stack) workspace->format_stack = stack_create(sizeof(text_format), 16);
    else {
        size_t c = stack_count(workspace->format_stack);
        for (u64 i = 0; i < c; i++)
            string_free(((text_format*)stack_get(workspace->format_stack,i))->link);
        stack_reset(workspace->format_stack);//TODO: do this in file as well
    }
    linked_list_for_each(workspace->workspace, workspace_source_list);
    workspace->header.formatting = stack_to_text_format(workspace->format_stack);
    // new_workspace_ptr = 0;
}

bool brain_workspace_handle_link(void *ctx, string_slice url){
    fs_stat out_stat = {};
    source_type_workspace *current_workspace = ctx;
    if (statf(url.data, &out_stat)){
        if (out_stat.type == entry_directory){
            linked_list_node_t *node = workspace_expands_dir(current_workspace,url);
            if (node){
                linked_list_remove(current_workspace->expanded_dirs, node);
            } else {
                linked_list_push(current_workspace->expanded_dirs, url.data);
            }
             brain_workspace_refresh(current_workspace);
        } else {
            current_buffer_open(url.data);
        }
    }
    return true;
}

void* brain_create_workspace_source(workspace_list *workspace){
    if (!workspace_type_id) brain_workspace_register();
    source_type_workspace *new_workspace = zalloc(sizeof(source_type_workspace));
    new_workspace->workspace = workspace;
    new_workspace->header.type = workspace_type_id;
    new_workspace->content_buf = buffer_create(0x100, buffer_can_grow);
    new_workspace->header.text_info = (text_field_info){
        .content = &new_workspace->content_buf,
        .placeholder = SLICE("EMPTY WORKSPACE"),
        .link_manager = brain_workspace_handle_link,
        .ctx = new_workspace
    };
    new_workspace->expanded_dirs = linked_list_create();
    brain_workspace_refresh(new_workspace);
    return new_workspace;
}

void* brain_create_default_workspace_source(){
    return brain_create_workspace_source(default_workspace());
}

bool brain_workspace_source_handle_kbd(buffer_source *ctx, kbd_event ev, u8 modifier){
    source_type_workspace *workspace = (source_type_workspace*)ctx;
    current_modifier = modifier;
    if (is_mod_pressed(KEY_LEFTCTRL, true)){
        if (ev.key == KEY_DOT && ev.type == KEY_PRESS){
            workspace->show_dotfiles = !workspace->show_dotfiles;
            buffer_wipe(&workspace->content_buf);
            brain_workspace_refresh(workspace);
            return true;
        }
    }
    return false;
}

bool brain_workspace_source_handle_mouse(buffer_source *ctx, mouse_data data, u8 modifier){
    return false;
}
