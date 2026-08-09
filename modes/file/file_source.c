#include "file_source.h"
#include "alloc/allocate.h"
#include "modes/modes.h"

int file_type_id = 0;

void brain_file_register(){
    file_type_id = register_mode_type(brain_create_file_source);
}

void* brain_create_file_source(){
    if (!file_type_id) brain_file_register();
    source_type_file *buf_ctx = zalloc(sizeof(source_type_file));
    buf_ctx->buf = buffer_create(0x100, buffer_can_grow);
    buf_ctx->header.type = file_type_id;
    
    buf_ctx->header.text_info = (text_field_info){
        .content = &buf_ctx->buf,
        .placeholder = SLICE_LIT("new buffer"),
        .multiline = true,
        .cursor_color = 0xFFFFFFFF
    };
    return buf_ctx;
}