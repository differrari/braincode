#include "tree_layout.h"
#include "uno.h"
#include "utils/indent.h"
#include "syscalls/syscalls.h"

tree_layout_node *root;
tree_layout_node *current_layout_node;

bool tree_select_node_id(int id){
    tree_layout_node *tree = tree_find_id(root,id);
    if (tree) {
        current_layout_node = tree;
        uno_focus(id);
    }
    return tree > 0;
}

void tree_draw_element(tree_layout_node *node){
    switch (node->type) {
        case tree_content: {
            on_draw_tree_leaf(node->id, node->ctx, current_layout_node == node);
        }
        break;
        case tree_horizontal:
            uno_begin_horizontal((node_info){.sizing_rule = size_fill});
                if (node->first_child) tree_draw_element(node->first_child);
            uno_end_horizontal();
        break;
        case tree_vertical:
            uno_begin_vertical((node_info){.sizing_rule = size_fill});
                if (node->first_child) tree_draw_element(node->first_child);
            uno_end_vertical();
        break;
    }
    if (node->next) tree_draw_element(node->next);
}

int tree_id = 1;

void *tree_page = 0;

tree_layout_node* tree_alloc(tree_layout_type type){
    if (!tree_page) tree_page = page_alloc(PAGE_SIZE);
    tree_layout_node *node = allocate(tree_page,sizeof(tree_layout_node),page_alloc);
    node->type = type;
    if (type == tree_content){
        node->id = tree_id++;
        node->ctx = popuplate_tree_leaf();
    }
    return node;
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

tree_layout_node* tree_find_id(tree_layout_node *node, int id){
    if (node->id == id) return node;
    if (node->first_child){
        if (node->first_child->id == id) return node->first_child;
        tree_layout_node *res = tree_find_id(node->first_child,id);
        if (res) return res;
    }
    if (node->next){
        if (node->next->id == id) return node->next;
        tree_layout_node *res = tree_find_id(node->next,id);
        if (res) return res;
    }
    return 0;
}

tree_layout_node* tree_find_next(tree_layout_node *node, bool ignore_children){
    if (!node) return 0;
    if (!ignore_children && node->first_child){
        if (node->first_child->type == tree_content){
            return node->first_child;
        } else {
            tree_layout_node *res = tree_find_next(node->first_child, false);
            if (res) return res;
        }
    }
    if (node->next){
        if (node->next->type == tree_content){
            return node->next;
        } else {
            tree_layout_node *res = tree_find_next(node->next, false);
            if (res) return res;
        }
    } else if (node->parent){
        return tree_find_next(node->parent, true);
    }
    return 0;
}

void tree_cycle_node(){
    current_layout_node = tree_find_next(current_layout_node, false);
    if (!current_layout_node){
        current_layout_node = root->type == tree_content ? root : tree_find_next(root, false);
    }
    if (current_layout_node){
        uno_focus(current_layout_node->id);
    }
}

int tree_deselect_current(){
    if (!current_layout_node) return 0;
    int current = current_layout_node->id;
    current_layout_node = 0;
    uno_focus(0);
    return current;
}

void new_tree_node(tree_create_options opts){
    bool horizontal = opts & 1;
    bool before = (opts >> 1) & 1;
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
    sibling->parent = new_node;
    if (before){
        new_node->first_child = sibling;
        sibling->next = current_layout_node;
        current_layout_node->next = 0;
    } else
        current_layout_node->next = sibling;
}

void debug_tree(tree_layout_node *node, int depth){
    switch (node->type) {
        case tree_content:
            print("%sBuffer %i %x",indent_by(depth),node->id,node);
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

void tree_debug(){
    debug_tree(root, 0);
}

void tree_draw_frame(){
    tree_draw_element(root);
}

void init_tree(draw_ctx *ctx, void (*custom_draw)(), int initial_id){
    if (initial_id) tree_id = initial_id;
    root = tree_alloc(tree_content);
    current_layout_node = root;
    set_document_view(custom_draw ?: tree_draw_frame, draw_ctx_rect(ctx));
    uno_focus(root->id);
}

void tree_close(tree_layout_node *node){
    if (!node) return;
    tree_layout_node *parent = node->parent;
    if (!parent) return;
    cleanup_tree_leaf(node->id,node->ctx);
    tree_layout_node *prev = 0;
    tree_layout_node *curr = parent->first_child;
    if (curr == node){
        if (prev) prev->next = node->next;
        else parent->first_child = node->next;
    }
    if (node == current_layout_node){
        current_layout_node = tree_find_next(root, false);
    }
}

tree_layout_node* tree_find(int id){
    return tree_find_id(root, id);
}

void tree_close_current(){
    tree_layout_node *old = current_layout_node;
    tree_cycle_node();
    tree_close(old);
}