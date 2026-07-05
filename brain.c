#include "syscalls/syscalls.h"
#include "tree_layout.h"
#include "input_keycodes.h"
#include "utils/indent.h"
#include "kbd_helper.h"

draw_ctx ctx;

typedef struct {
    text_field_info tf_info;
    buffer buf;
} buffer_ctx;

document_node* on_draw_tree_leaf(int id, void *ctx, bool selected){
    uno_begin_vertical((node_info){.sizing_rule = size_fill});
    buffer_ctx *buf_ctx = ctx;
    document_node* doc_node = uno_text_field(id, (node_info){.sizing_rule = size_fill, .text_wrap_policy = wrap_word, .fg_color = 0xFFFFFFFF }, &buf_ctx->tf_info);
    if (selected)
        uno_label((node_info){.sizing_rule = size_fit, .fg_color = 0xFF887766, .bg_color = 0x55554433 }, doc_text_footnote, SLICE_LIT("CURRENT"));
    uno_end_vertical();
    return doc_node;
}

void* popuplate_tree_leaf(){
    buffer_ctx *buf_ctx = zalloc(sizeof(buffer_ctx));
    buf_ctx->buf = buffer_create(0x100, buffer_can_grow);
    
    buf_ctx->tf_info = (text_field_info){
        .content = &buf_ctx->buf,
        .placeholder = SLICE_LIT("new buffer")
    };
    return buf_ctx;
}

int main(int argc, char *argv[]){
    print("Welcome to brain");

    ctx.width = 1920;
    ctx.height = 1080;
    request_app_ctx(&ctx);

    init_tree(&ctx);
    // set_document_view(tree_draw_frame, draw_ctx_rect(ctx));

    while (!should_close_ctx()){
        fb_clear(&ctx, 0);
        tree_draw(&ctx);
        commit_draw_ctx(&ctx);
        kbd_event ev = {};
        if (read_event(&ev)){
            if (!handle_modifier(&ev)){
                if (current_modifier == KEY_LEFTCTRL || current_modifier == KEY_RIGHTCTRL){
                    if (ev.type == KEY_PRESS){
                        switch (ev.key){
                            case KEY_UP: new_tree_node(tree_create_before | tree_create_vertical); break;
                            case KEY_DOWN: new_tree_node(tree_create_after | tree_create_vertical); break;
                            case KEY_LEFT: new_tree_node(tree_create_before | tree_create_horizontal); break;
                            case KEY_RIGHT: new_tree_node(tree_create_after | tree_create_horizontal); break;
                            case KEY_ESC: halt(0);
                        }
                        if (ev.key == KEY_TAB) tree_cycle_node();
                    }
                }
                else tree_dispatch_kbd(ev, current_modifier);
                tree_refresh();
                if (ev.type == KEY_PRESS) tree_debug();
            }
        }
        mouse_data data = {};
        get_mouse_status(&data);
        tree_dispatch_mouse(data, 0);
    }

    return 0;
    
}