#include "file_source.h"
#include "alloc/allocate.h"
#include "data/struct/bt_tree.h"
#include "files/buffer.h"
#include "files/helpers.h"
#include "modes/modes.h"
#include "string/string.h"
#include "uno.h"
#include "parser/parser.h"
#include "parser/ast.h"

int file_type_id = 0;

void brain_file_register(){
    file_type_id = register_mode_type(brain_create_file_source);
}

void* brain_create_file_source(){
    if (!file_type_id) brain_file_register();
    source_type_file *buf_ctx = zalloc(sizeof(source_type_file));
    buf_ctx->buf = buffer_create(0x100, buffer_can_grow);
    buf_ctx->header.type = file_type_id;
    // buf_ctx->tree = uno_text_make_piece_tree();
    // buf_ctx->gap = buffer_create(0x100, buffer_can_grow);

    buf_ctx->header.text_info = (text_field_info){
        .content = &buf_ctx->buf,
        .gap = &buf_ctx->gap,
        .piece_tree = &buf_ctx->tree,
        .placeholder = SLICE_LIT("new buffer"),
        .multiline = true,
        .cursor_color = 0xFFFFFFFF,
        .cursor_limit = 1,
        .cursor_count = 1,
        .cursors = buf_ctx->header.cursors
    };
    return buf_ctx;
}

void brain_parse_format(source_type_file *file, string_slice slice){
    if (sizeof(parse_result) + sizeof(chunk_array_t) > slice.length) return;
    // print("Slice size %x",slice.length);
    parse_result *res = (parse_result*)slice.data;
    chunk_array_t *arr = (chunk_array_t*)((uptr)res + sizeof(parse_result));
    if (arr->item_size * arr->count + sizeof(parse_result) + sizeof(chunk_array_t) > slice.length) return;
    // print("Parse result %i. Arr %llx %llx %i",res->result,res,arr,arr->count);
    arr_stack_t *stack = stack_create(sizeof(text_format), arr->count);
    for (u64 i = 0; i < arr->count; i++){
        // print("Inside for loop %i of %i",i,arr->count);
        ast_node *node = chunk_array_get(arr, i);
        // print("Node %i. %i,%i,%i",node->t.kind,node->rule,node->option,node->sequence);
        if (node->t.kind){
            text_format fmt = { .bounds = {node->t.pos,node->t.length} };
            // print("Color for token %i %i,%i",node->t.kind,node->t.pos,node->t.length);
            switch (node->t.kind){
                case TOK_IDENTIFIER: fmt.foreground = 0xFFb4dd13; break;
                case TOK_STRING: fmt.foreground = 0xFFb4dd13; break;
                case TOK_NUMBER: fmt.foreground = 0xFFb4dd13; break;
                default: break;
            }
            stack_push(stack,&fmt);
        }
    }
    file->header.formatting = stack_to_text_format(stack);
}

void brain_file_set_content(source_type_file *file, string_slice slice, char *path, data_signature signature){
    file->buf = (buffer){
        .buffer = slice.data,
        .buffer_size = slice.length,
        .limit = slice.length,
        .options = buffer_can_grow,
        .data_type = signature
    };
    if (file->tree.root) uno_text_piece_identity(&file->header.text_info);
    if (file->gap.buffer) file->gap.buffer_size = 0;
    if (file->path.mem_length) string_free(file->path);
    // if (signature == DATA_SIGNATURE("LISP")){
    //     brain_parse_format(file, read_file_slice("/lfsp/0/syntax"));
    // }
    file->path = string_from_literal(path);
    uno_refresh();
}

bool brain_file_source_handle_kbd(buffer_source *ctx, kbd_event ev, u8 modifier){
    return false;
}