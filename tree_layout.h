#pragma once

///Wrapper around the uno library to create similar stack/based views, editable at runtime
//Uno's document nodes are ephemeral, they can be destroyed by a refresh. These are meant to be persistent representations for uno to build nodes off them.

#include "uno.h"
#include "types.h"
#include "graphic_types.h"

typedef enum {
    tree_content,
    tree_horizontal,
    tree_vertical
} tree_layout_type;

typedef struct tree_layout_node {
    int id;
    tree_layout_type type;
    void *ctx;
    struct tree_layout_node *first_child;
    struct tree_layout_node *next;
    struct tree_layout_node *parent;
} tree_layout_node;

tree_layout_node* tree_find_id(tree_layout_node *node, int id);

extern void* popuplate_tree_leaf();
extern document_node* on_draw_tree_leaf(int id, void *ctx, bool selected);
extern void cleanup_tree_leaf(int id, void*ctx);

typedef enum {
    tree_create_vertical = 0 << 0,
    tree_create_horizontal = 1 << 0,
    tree_create_after = 0 << 1,
    tree_create_before = 1 << 1,
} tree_create_options;

void new_tree_node(tree_create_options);

bool tree_select_node_id(int id);

static inline void tree_select_node(tree_layout_node *node){
    tree_select_node_id(node->id);
}

void tree_draw_frame();
void init_tree(draw_ctx *ctx, void (*custom_draw)(), int initial_id);

void draw_tree(draw_ctx *ctx);

void tree_debug();

void tree_cycle_node();

void tree_close(tree_layout_node *node);

void tree_close_current();

int tree_deselect_current();

tree_layout_node* tree_find(int id);

static inline void tree_draw(draw_ctx *ctx){
    uno_draw(ctx);
}

static inline bool tree_dispatch_kbd(kbd_event ev, u8 modifier){
    return uno_dispatch_kbd(ev, modifier);
}

static inline void tree_refresh(){
    uno_refresh();
}

static inline bool tree_dispatch_mouse(mouse_data mouse, u8 modifier){
    return uno_dispatch_mouse(mouse, modifier);
}
