#include "workspace_source.h"
#include "modes/modes.h"
#include "utils/indent.h"
#include "files/helpers.h"
#include "kbd_helper.h"
#include "input_keycodes.h"

int workspace_type_id = 0;

void brain_workspace_register(){
    workspace_type_id = register_mode_type(brain_create_default_workspace_source);
}

source_type_workspace *new_workspace_ptr;
int indentation = 0;

void workspace_source_traverse(const char *directory, const char *file){
    size_t len = strlen(file);
    if (!new_workspace_ptr->show_dotfiles && len && *file == '.') return;
    buffer_write(&new_workspace_ptr->content_buf, "%s",indent_by(indentation));
    fs_stat out_stat = {};
    string s = string_format("%s/%s",directory,file);
    if (statf(s.data, &out_stat)){
        range_t r = { .start = new_workspace_ptr->content_buf.cursor, .size = len };
        stack_push(new_workspace_ptr->format_stack, &(text_format){
            .foreground = out_stat.type == entry_directory ? 0xff3232cc : 0xffcccccc,
            .bounds = r,
        });
    }
    buffer_write(&new_workspace_ptr->content_buf, "%s\n",file);
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
    if (!workspace->format_stack) workspace->format_stack = stack_create(sizeof(text_format), 16);
    else stack_reset(workspace->format_stack);//TODO: do this in file as well
    linked_list_for_each(workspace->workspace, workspace_source_list);
    workspace->header.formatting = stack_to_text_format(workspace->format_stack);
    new_workspace_ptr = 0;
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
    };
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