#include "file_source.h"
#include "alloc/allocate.h"
#include "data/struct/bt_tree.h"
#include "files/buffer.h"
#include "modes/modes.h"
#include "string/string.h"
#include "uno.h"

int file_type_id = 0;

void brain_file_register(){
    file_type_id = register_mode_type(brain_create_file_source);
}

void* brain_create_file_source(){
    if (!file_type_id) brain_file_register();
    source_type_file *buf_ctx = zalloc(sizeof(source_type_file));
    buf_ctx->buf = buffer_create(0x100, buffer_can_grow);
    buf_ctx->header.type = file_type_id;
    buf_ctx->tree = uno_text_make_piece_tree();
    buf_ctx->gap = buffer_create(0x100, buffer_can_grow);

    buf_ctx->header.text_info = (text_field_info){
        .content = &buf_ctx->buf,
        .gap = &buf_ctx->gap,
        .piece_tree = &buf_ctx->tree,
        .placeholder = SLICE_LIT("new buffer"),
        .multiline = true,
        .cursor_color = 0xFFFFFFFF
    };
    return buf_ctx;
}

void brain_file_set_content(source_type_file *file, string_slice slice, char *path, data_signature signature){
    file->buf = (buffer){
        .buffer = slice.data,
        .buffer_size = slice.length,
        .limit = slice.length,
        .options = buffer_can_grow,
        .data_type = signature
    };
    uno_text_piece_identity(&file->header.text_info);
    file->gap.buffer_size = 0;
    if (file->path.mem_length) string_free(file->path);
    file->path = string_from_literal(path);
}
