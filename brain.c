#include "uno.h"
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

tree_layout_node *root;
tree_layout_node *current_layout_node;

void draw_element(tree_layout_node *node){
    switch (node->type) {
        case tree_content:
            if (node == current_layout_node){
                uno_begin_vertical((node_info){.sizing_rule = size_fill});
            }
            buffer_ctx *buf_ctx = node->ctx;
            uno_text_field(node->id, (node_info){.sizing_rule = size_fill, .text_wrap_policy = wrap_word, .fg_color = 0xFFFFFFFF }, &buf_ctx->tf_info);
            if (node == current_layout_node){
                    uno_label((node_info){.sizing_rule = size_fit, .fg_color = 0xFF887766, .bg_color = 0x55554433 }, doc_text_footnote, SLICE_LIT("CURRENT"));
                uno_end_vertical();
            }
        break;
        case tree_horizontal:
            uno_begin_horizontal((node_info){.sizing_rule = size_fill});
                if (node->first_child) draw_element(node->first_child);
            uno_end_horizontal();
        break;
        case tree_vertical:
            uno_begin_vertical((node_info){.sizing_rule = size_fill});
                if (node->first_child) draw_element(node->first_child);
            uno_end_vertical();
        break;
    }
    if (node->next) draw_element(node->next);
}

void draw_frame(){
    draw_element(root);
}

int tree_id = 1;

tree_layout_node* tree_alloc(tree_layout_type type){
    tree_layout_node *node = zalloc(sizeof(tree_layout_node));
    node->id = tree_id++;
    node->type = type;
    if (type == tree_content){
        buffer_ctx *buf_ctx = zalloc(sizeof(buffer_ctx));
        node->ctx = buf_ctx;
        buf_ctx->buf = buffer_create(0x100, buffer_can_grow);
        
        buf_ctx->tf_info = (text_field_info){
            .content = &buf_ctx->buf,
            .placeholder = SLICE_LIT("new buffer")
        };
    }
    return node;
}

void init_tree(){
    root = tree_alloc(tree_content);
    root->index = 1;
    current_layout_node = root;
}

void tree_replace_child_in_parent(tree_layout_node *child, tree_layout_node *new_node){
    tree_layout_node *parent = child->parent;
    if (!parent) return;
    tree_layout_node *prev = 0;
    new_node->parent = parent;
    for (tree_layout_node *c = parent->first_child; c; c = c->next){
        if (c == child){
            if (prev)
                prev->next = new_node;
            else 
                parent->first_child = new_node;
            new_node->next = c->next;
            return;
        }
        prev = c;
    }
}

tree_layout_node* find_next(tree_layout_node *node, bool ignore_children){
    if (!ignore_children && node->first_child){
        if (node->first_child->type == tree_content){
            return node->first_child;
        } else {
            tree_layout_node *res = find_next(node->first_child, false);
            if (res) return res;
        }
    }
    if (node->next){
        if (node->next->type == tree_content){
            return node->next;
        } else {
            tree_layout_node *res = find_next(node->next, false);
            if (res) return res;
        }
    } else if (node->parent){
        return find_next(node->parent, true);
    }
    return 0;
}

void next_node(){
    current_layout_node = find_next(current_layout_node, false);
    if (!current_layout_node){
        current_layout_node = find_next(root, false);
    }
    if (current_layout_node){
        uno_focus(current_layout_node->id);
    }
}

void new_layout_buf(bool horizontal, bool before){
    if (!current_layout_node) current_layout_node = root;
    tree_layout_type type = horizontal ? tree_horizontal : tree_vertical;
    tree_layout_node *new_node = tree_alloc(type);
    new_node->first_child = current_layout_node;
    if (current_layout_node->parent)
        tree_replace_child_in_parent(current_layout_node, new_node);
    else 
        root = new_node;
    current_layout_node->parent = new_node;
    tree_layout_node *sibling = tree_alloc(tree_content);
    sibling->index = 1;
    if (before){
        new_node->first_child = sibling;
        sibling->next = current_layout_node;
    } else
        current_layout_node->next = sibling;
    sibling->parent = new_node;
}

void debug_tree(tree_layout_node *node, int depth){
    switch (node->type) {
        case tree_content:
            print("%sBuffer %i %x",indent_by(depth),node->index,node);
        break;
        case tree_horizontal:
            print("%sHorizontal %x",indent_by(depth), node);
            if (node->first_child) debug_tree(node->first_child, depth+1);
        break;
        case tree_vertical:
            print("%sVertical %x",indent_by(depth), node);
            if (node->first_child) debug_tree(node->first_child, depth+1);
        break;
    }
    if (node->next) debug_tree(node->next, depth);
}

int main(int argc, char *argv[]){
    print("Welcome to brain");

    ctx.width = 1920;
    ctx.height = 1080;
    request_app_ctx(&ctx);

    init_tree();
    set_document_view(draw_frame, draw_ctx_rect(&ctx));

    uno_focus(1);

    while (!should_close_ctx()){
        fb_clear(&ctx, 0);
        uno_draw(&ctx);
        commit_draw_ctx(&ctx);
        kbd_event ev = {};
        if (read_event(&ev)){
            if (!handle_modifier(&ev)){
                if (current_modifier == KEY_LEFTCTRL || current_modifier == KEY_RIGHTCTRL){
                    if (ev.type == KEY_PRESS){
                        switch (ev.key){
                            case KEY_UP: new_layout_buf(false, true); break;
                            case KEY_DOWN: new_layout_buf(false, false); break;
                            case KEY_LEFT: new_layout_buf(true, true); break;
                            case KEY_RIGHT: new_layout_buf(true, false); break;
                        }
                        if (ev.key == KEY_TAB) next_node();
                    }
                }
                else uno_dispatch_kbd(ev, current_modifier);
                uno_refresh();
                if (ev.type == KEY_PRESS) debug_tree(root, 0);
            }
        }
        mouse_data data = {};
        get_mouse_status(&data);
        uno_dispatch_mouse(data, 0);
    }

    return 0;
    
}