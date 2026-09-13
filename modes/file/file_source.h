#pragma once

#include "files/buffer.h"
#include "string/string.h"
#include "brain.h"

typedef struct {
    buffer_source header;
    buffer buf;
    buffer gap;
    bt_tree tree;
    string path;
} source_type_file;

void* brain_create_file_source();

void brain_file_register();

void brain_file_set_content(source_type_file *file, string_slice slice, char *path, data_signature signature);

extern int file_type_id;
