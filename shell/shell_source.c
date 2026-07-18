#include "shell_source.h"
#include "shell/sheldon/sheldon.h"
#include "syscalls/syscalls.h"
#include "interpreter/repl.h"
#include "debug_print.h"

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

bool is_script(string_slice cmd){
    if (cmd.length < 3 || !cmd.data) return false;
    if (slice_lit_match((string_slice){cmd.data, 2}, "#!", true)){
        return true;
    }
    if (*cmd.data == '(') return true;
    return false;
}

buffer sexp_to_string(codegen exp, codegen args){
    string_slice cmd = car_id(exp);
    if (!cmd.data || !cmd.length) return (buffer){};
    buffer buf = buffer_create(0x100, buffer_can_grow);
    buffer_write(&buf, "%v ", cmd);
    while (args.ptr) {
        codegen arg = car(args);
        if (!arg.ptr || arg.type != sem_rule_lisp_val) break;
        lisp_val_code *code = arg.ptr;
        switch (code->type) {
            case car_identifier:
                buffer_write(&buf, "%v ",code->val);
                break;
            case car_string:
                if (code->val.length < 2) break;
                buffer_write(&buf, "%v ",(string_slice){ code->val.data + 1, code->val.length-2 });
                break;
            case car_num:
                buffer_write(&buf, "%i ",code->number);
                break;
            case car_true:
            case car_none:
            break;
        }
        args = cdr(args);
    }
    return buf;
}

source_type_shell *current_shell_ctx = 0;

codegen shell_imaginal_fncall(codegen exp, codegen args, codegen *env){
    if (!current_shell_ctx) return nil_exp;
    buffer cmd = sexp_to_string(exp, args);
    string_slice s = slice_from_buffer(&cmd);
    run_cmd(current_shell_ctx->shell, s);
    return nil_exp;
}

void shell_handle_cmd(void *ctx, string_slice cmd){
    source_type_shell *buf_ctx = ctx;
    if (is_script(cmd)){
        imaginal_fallback_fncall = shell_imaginal_fncall;
        current_shell_ctx = buf_ctx;
        repl_run(cmd, true);
        current_shell_ctx = 0;
    } else {
        run_cmd(buf_ctx->shell, cmd);
    }
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