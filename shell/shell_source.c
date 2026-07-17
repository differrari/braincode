#include "shell_source.h"
#include "shell/sheldon/sheldon.h"
#include "syscalls/syscalls.h"

int shell_type_id = 0;

void put_char(shell_handle *handle, char c){
    source_type_shell *buf_ctx = handle->owner;
    buffer_write_lim(&buf_ctx->buf, &c, 1);
}

void flush(shell_handle *handle){
    uno_refresh();
}

void clear(shell_handle *handle){
    source_type_shell *buf_ctx = handle->owner;
}

void bell(shell_handle *handle){
    source_type_shell *buf_ctx = handle->owner;
}

void ascii_cmd(shell_handle *handle, char cmd, u16 proc_id){
    source_type_shell *buf_ctx = handle->owner;
}

void console_ctrl(shell_handle *handle, console_ctrls ctrl){
    source_type_shell *buf_ctx = handle->owner;
}

shell_bindings terminal_bindings = (shell_bindings){
    .console_output = put_char,
    .console_flush = flush,
    .console_clean = clear,
    .console_bell = bell,
    .console_ascii_cmd = ascii_cmd,
    .console_control = console_ctrl
};

void shell_handle_cmd(void *ctx, string_slice cmd){
    source_type_shell *buf_ctx = ctx;
    run_cmd(buf_ctx->shell, cmd);
}

void* brain_create_shell_source(){
    if (!shell_type_id) shell_type_id = register_source_type();
    source_type_shell *buf_ctx = zalloc(sizeof(source_type_shell));
    buf_ctx->header.type = shell_type_id;
    buf_ctx->buf = buffer_create(0x100, buffer_can_grow);
    buf_ctx->header.command_handler = shell_handle_cmd;
    shell_handle *handle = create_sheldon(terminal_bindings, buf_ctx, 0);
    current_shell = handle;
    buf_ctx->header.text_info = (text_field_info){
        .content = &handle->out_buffer,
        .placeholder = SLICE(""),
        .multiline = true,
        .cursor_color = 0xFFFFFFFF
    };
    buf_ctx->shell = handle;
    return buf_ctx;
}