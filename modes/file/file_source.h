#pragma once

#include "files/buffer.h"
#include "string/string.h"
#include "brain.h"

typedef struct {
    buffer_source header;
    buffer buf;
    string path;
} source_type_file;

void* brain_create_file_source();

void brain_file_register();

extern int file_type_id;