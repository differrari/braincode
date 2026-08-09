#include "syscalls/syscalls.h"
#include "tree_layout.h"
#include "input_keycodes.h"
#include "kbd_helper.h"
#include "files/helpers.h"
#include "math/rng.h"
#include "brain.h"
#include "modes/file/file_source.h"
#include "modes/shell/shell_source.h"
#include "modes/modes.h"
#include "debug_print.h"
#include "shell/sheldon/builtins.h"
#include "environment/env_types.h"

draw_ctx ctx;

typedef enum {
    invalid_id = 0,
    command_buffer_id,

    custom_buffers_count,
} custom_buffer_ids;

color current_color = 0xFF362872;

bool command_buffer_forward_input = false;

source_type_shell *command_buffer;
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
    source_type_file *buf_ctx = ctx;
    document_node* doc_node = uno_text_field(id, (node_info){.sizing_rule = size_fill, .text_wrap_policy = wrap_word_preserve_indent, .fg_color = 0xFFFFFFFF }, &buf_ctx->header.text_info);
    doc_node->input.mouse_input = buf_mouse;
    if (selected)
        uno_label((node_info){.sizing_rule = size_fit, .fg_color = 0xFF847237, .bg_color = 0x55736126 }, doc_text_footnote, SLICE_LIT("CURRENT"));
    uno_end_vertical();
    return doc_node;
}

void save_current_buffer(){
    if (current_buf < custom_buffers_count) return;
    tree_layout_node *node = tree_find(current_buf);
    source_type_file *bctx = node->ctx;
    if (bctx->path.data)
        write_full_file(bctx->path.data, bctx->buf.buffer, bctx->buf.buffer_size);
}

void* buf_page = 0;

void* popuplate_tree_leaf(){
    if (!buf_page) buf_page = page_alloc(PAGE_SIZE);
    return create_mode_buffer_default();
}

extern void cleanup_tree_leaf(int id, void*ctx){
    release(ctx);
}

void open_command_buffer(){
    uno_focus(command_buffer_id);
    in_command = true;
}

void close_command_buffer(int new_buf){
    tree_select_node_id(new_buf);
    uno_focus(new_buf);
    in_command = false;
}

void refresh_buffer_fwd(){
    if (command_buffer_forward_input)
        command_buffer->header.text_info.placeholder = SLICE(">");
    else 
        command_buffer->header.text_info.placeholder = SLICE("|>");
    buffer_wipe(&command_buffer->shell->out_buffer);
}

bool handle_command(string_slice cmd){
    if (slice_lit_match(cmd, "input", true)){
        command_buffer_forward_input = !command_buffer_forward_input;
        refresh_buffer_fwd();
        return true;
    }
    if (command_buffer_forward_input){
        if (current_buf < custom_buffers_count) return false;
        tree_layout_node *node = tree_find(current_buf);
        buffer_source *bctx = node->ctx;
        bctx->command_handler(bctx,cmd);
        buffer_wipe(&command_buffer->shell->out_buffer);
        return true;
    } else if (command_buffer->header.command_handler) {
        command_buffer->header.command_handler(command_buffer,cmd);
        buffer_wipe(&command_buffer->shell->out_buffer);
        return true;
    }
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
        return handle_command(slice_from_buffer(&command_buffer->shell->out_buffer));
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
    VERTICAL(((node_info){.sizing_rule = size_fill, .bg_color = current_color}), {
        tree_draw_frame();
        document_node* doc_node = uno_text_field(command_buffer_id, (node_info){.bg_color = current_color, .sizing_rule = size_relative, .percentage = 0.05f, .fg_color = 0xFFFFFFFF }, &command_buffer->header.text_info);
        doc_node->input.mouse_input = command_buffer_mouse;
        doc_node->input.keyboard_input = command_buffer_kbd;
    });
}

void switch_current_buffer_mode(char *str){
    if (current_buf < custom_buffers_count) return;
    debug_print("Switching to %i to %s mode",current_buf,str);
    
    tree_layout_node *node = tree_find(current_buf);
    if (!node){ debug_print("Couldn't find %i buffer",current_buf); return; }
    buffer_source *bctx = node->ctx;

    release(bctx);//TODO: proper cleanup

    if (strcmp("shell", str) == 0){
        node->ctx = brain_create_shell_source(0, false);
    } else if (strcmp("file", str) == 0){
        node->ctx = brain_create_file_source();
    }
    
}
SHELLEY_CMD_FWD_1ARG(mode, switch_current_buffer_mode, string);

void current_buffer_open(char *str){
    if (current_buf < custom_buffers_count) return;
    tree_layout_node *node = tree_find(current_buf);
    buffer_source *bctx = node->ctx;
    if (bctx->type != file_type_id){
        switch_current_buffer_mode("file");
        bctx = node->ctx;
    }
    fs_stat stat = {};
    // if (statf(str, &stat)){
        // if (stat.type != entry_file) return;//TODO: we should also handle directories
        size_t size = 0;
        char *contents = read_full_file(str, &size);
        source_type_file *bfile = (source_type_file*)bctx;
        bfile->buf = (buffer){
            .buffer = contents,
            .buffer_size = size,
            .limit = size,
            .options = buffer_can_grow,
            .data_type = stat.data_type
        };
        bfile->path = string_from_literal(str);
    // }
}
SHELLEY_CMD_FWD_1ARG(open, current_buffer_open, string);

void register_command_buffer_builtins(shell_handle *handle){
    REG_BUILTIN(mode);
    REG_BUILTIN(open);
}

bool is_terminal_mode = true;

void terminal_mode(){
    mode_set_default(shell_type_id);
    command_buffer_forward_input = true;
    is_terminal_mode = true;
}

void parse_args(int argc, char*argv[]){
    for (int i = 1; i < argc; i++){
        if (slice_lit_match(SLICE("-terminal"), argv[i], true) || slice_lit_match(SLICE("-t"), argv[i], true)){
            terminal_mode();
        }
    }
}

int main(int argc, char *argv[]){
    print("Welcome to brain");

    brain_file_register();
    brain_shell_register();

    parse_args(argc, argv);

#if TERMINAL
    terminal_mode();
#endif

#ifdef CROSS
    ctx.width = 1920;
    ctx.height = 1080;
#endif
    request_app_ctx(&ctx);

    if (is_terminal_mode)
        env_set_window_info(&window_info_name_lit("terminal"));
    else 
        env_set_window_info(&window_info_name_lit("brain"));

    // command_buffer.buf = buffer_create(0x100, buffer_can_grow);

    command_buffer = brain_create_shell_source(register_command_buffer_builtins, true);
    command_buffer->header.text_info = (text_field_info){
        .multiline = false,
        .cursor_color = 0xFFFFFFFF,
        .content = &command_buffer->shell->out_buffer,
        .placeholder = SLICE(""),
    };
    refresh_buffer_fwd();
    buffer_wipe(&command_buffer->shell->out_buffer);
    
    init_tree(&ctx, brain_update_view, custom_buffers_count);

    if (is_terminal_mode) open_command_buffer();

    rng_t rng = {};
    rng_seed(&rng, get_time());

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
                            case KEY_ESC: halt(0); break;
                            case KEY_TAB: current_buf = tree_cycle_node(); in_command = false; break;
                            case KEY_X: toggle_command_buffer(); break;
                            case KEY_S: save_current_buffer(); break;
                            case KEY_W: tree_close_current(); break;
                        }
                    } else if (is_mod_pressed(KEY_LEFTALT, true)){
                        switch (ev.key) {
                            case KEY_S: save_current_buffer(); break;
                            case KEY_W: tree_close_current(); break;
                            case KEY_ENTER: current_color = rng_next32(&rng); current_color |= 0xFF000000; print("Current color: #%#X",current_color); break;
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