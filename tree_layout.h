#pragma once

typedef enum {
    tree_content,
    tree_horizontal,
    tree_vertical
} tree_layout_type;

typedef struct tree_layout_node {
    int id;
    tree_layout_type type;
    int index;
    void *ctx;
    struct tree_layout_node *first_child;
    struct tree_layout_node *next;
    struct tree_layout_node *parent;
} tree_layout_node;