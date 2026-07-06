#include "syscalls/syscalls.h"
#include "tree_layout.h"
#include "input_keycodes.h"
#include "kbd_helper.h"
#include "files/helpers.h"

draw_ctx ctx;

typedef enum {
    invalid_id = 0,
    command_buffer_id,

    custom_buffers_count,
} custom_buffer_ids;

typedef struct {
    text_field_info tf_info;
    buffer buf;
    string path;
} buffer_ctx;

buffer_ctx command_buffer;
int current_buf = custom_buffers_count;
bool in_command = false;

void close_command_buffer(int new_buf);

bool buf_mouse(document_node *node, mouse_data data, u8 modifier){
    if (mouse_button_down(&data, LMB))
        if (in_command)
            close_command_buffer(node->input.tag);
    return uno_text_field_mouse(node, data, modifier);
}

document_node* on_draw_tree_leaf(int id, void *ctx, bool selected){
    uno_begin_vertical((node_info){.sizing_rule = size_fill});
    buffer_ctx *buf_ctx = ctx;
    document_node* doc_node = uno_text_field(id, (node_info){.sizing_rule = size_fill, .text_wrap_policy = wrap_word_preserve_indent, .fg_color = 0xFFFFFFFF }, &buf_ctx->tf_info);
    doc_node->input.mouse_input = buf_mouse;
    if (selected)
        uno_label((node_info){.sizing_rule = size_fit, .fg_color = 0xFF887766, .bg_color = 0x55554433 }, doc_text_footnote, SLICE_LIT("CURRENT"));
    uno_end_vertical();
    return doc_node;
}

void save_current_buffer(){
    if (current_buf < custom_buffers_count) return;
    tree_layout_node *node = tree_find(current_buf);
    buffer_ctx *bctx = node->ctx;
    print("Save %i to %S",current_buf,bctx->path);
    if (bctx->path.data)
        write_full_file(bctx->path.data, bctx->buf.buffer, bctx->buf.buffer_size);
}

void* buf_page = 0;

void* popuplate_tree_leaf(){
    if (!buf_page) buf_page = page_alloc(PAGE_SIZE);
    buffer_ctx *buf_ctx = allocate(buf_page,sizeof(buffer_ctx),page_alloc);
    buf_ctx->buf = buffer_create(0x100, buffer_can_grow);
    
    buf_ctx->tf_info = (text_field_info){
        .content = &buf_ctx->buf,
        .placeholder = SLICE_LIT("new buffer"),
        .multiline = true,
        .cursor_color = 0xFFFFFFFF
    };
    return buf_ctx;
}

extern void cleanup_tree_leaf(int id, void*ctx){
    release(ctx);
}

void open_command_buffer(){
    current_buf = tree_deselect_current();
    uno_focus(command_buffer_id);
    in_command = true;
}

void close_command_buffer(int new_buf){
    tree_select_node_id(new_buf);
    uno_focus(new_buf);
    in_command = false;
}

bool handle_command(string_slice cmd){
    if (current_buf < custom_buffers_count) return false;
    tree_layout_node *node = tree_find(current_buf);
    buffer_ctx *bctx = node->ctx;
    //TODO: check for unsaved changes in the path
    buffer_destroy(&bctx->buf);
    string path = string_from_slice(cmd);
    size_t size = 0;
    char *f = read_full_file(path.data, &size);
    bctx->buf = (buffer){
        .buffer = f,
        .buffer_size = size,
        .limit = size,
        .options = buffer_can_grow,
        .cursor = 0,
    };
    bctx->path = path;
    print("Open file %v",cmd);
    return true;
}

void toggle_command_buffer(){
    if (in_command){
        close_command_buffer(current_buf);
    } else {
        open_command_buffer();
    }
}

bool command_buffer_kbd(document_node *node, kbd_event event, u8 modifier){
    if (event.type == KEY_PRESS && event.key == KEY_ENTER){
        return handle_command(slice_from_buffer(&command_buffer.buf));
    }
    return uno_text_field_input(node, event, modifier);
}

bool command_buffer_mouse(document_node *node, mouse_data data, u8 modifier){
    if (mouse_button_down(&data, LMB)){
        open_command_buffer();
    }
    return uno_text_field_mouse(node, data, modifier);
}

void brain_update_view(){
    VERTICAL(((node_info){.sizing_rule = size_fill, .bg_color = 0xFF362872}), {
        tree_draw_frame();
        document_node* doc_node = uno_text_field(command_buffer_id, (node_info){.bg_color = 0xFF376298, .sizing_rule = size_relative, .percentage = 0.05f, .fg_color = 0xFFFFFFFF }, &command_buffer.tf_info);
        doc_node->input.mouse_input = command_buffer_mouse;
        doc_node->input.keyboard_input = command_buffer_kbd;
    });
}

int main(int argc, char *argv[]){
    print("Welcome to brain");

    ctx.width = 1920;
    ctx.height = 1080;
    request_app_ctx(&ctx);

    command_buffer.buf = buffer_create(0x100, buffer_can_grow);
    command_buffer.tf_info = (text_field_info){
        .multiline = false,
        .cursor_color = 0xFFFFFFFF,
        .content = &command_buffer.buf,
        .placeholder = SLICE("> "),
    };

    init_tree(&ctx, brain_update_view, custom_buffers_count);

    while (!should_close_ctx()){
        fb_clear(&ctx, 0);
        tree_draw(&ctx);
        commit_draw_ctx(&ctx);
        kbd_event ev = {};
        if (read_event(&ev)){
            if (!handle_modifier(&ev)){
                if (ev.type == KEY_PRESS){
                    if (is_mod_pressed(KEY_LEFTCTRL, true)){
                        switch (ev.key){
                            case KEY_UP: new_tree_node(tree_create_before | tree_create_vertical); break;
                            case KEY_DOWN: new_tree_node(tree_create_after | tree_create_vertical); break;
                            case KEY_LEFT: new_tree_node(tree_create_before | tree_create_horizontal); break;
                            case KEY_RIGHT: new_tree_node(tree_create_after | tree_create_horizontal); break;
                            case KEY_ESC: halt(0);
                            case KEY_TAB: tree_cycle_node();
                            case KEY_X: toggle_command_buffer(); break;
                            case KEY_S: save_current_buffer(); break;
                            case KEY_W: tree_close_current(); break;
                        }
                    } else if (is_mod_pressed(KEY_LEFTALT, true)){
                        switch (ev.key) {
                            case KEY_S: save_current_buffer(); break;
                            case KEY_W: tree_close_current(); break;
                        }
                    }
                    else tree_dispatch_kbd(ev, current_modifier);
                    tree_refresh();
                }
            }
        }
        mouse_data data = {};
        get_mouse_status(&data);
        tree_dispatch_mouse(data, 0);
    }

    return 0;
    
}