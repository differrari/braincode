#include "syscalls/syscalls.h"
#include "files/helpers.h"
#include "files/buffer.h"
#include "input_keycodes.h"
#include "uno.h"
#include "math/math.h"
#include "ui/color/color.h"
#include "kbd_helper.h"
#include "memory/memory.h"

enum { no_input, main_code } input_focus;

//TODO: file selector

struct {
    u32 bg;
    u32 fg;
} palette;

buffer code;

text_field_info tf_info;

char *file_path = "/shared/projects/code/braincode/main.c";

stack_t *format_rules;

void ui(){
    HORIZONTAL(((node_info){.sizing_rule = size_fill}),{
        uno_text_field(main_code, (node_info){.sizing_rule = size_fill,.fg_color = palette.fg, .text_formatting = stack_to_text_format(format_rules) }, &tf_info);
    });
}

bool save_file(){
    return write_full_file(file_path, code.buffer, code.buffer_size);
}

int main(int argc, char *argv[]){
    
    if (argc){
        file_path = argv[0];
    }
    
    size_t size = 0x1000;
    char *contents = read_full_file(file_path, &size);
    
    if (!contents) contents = zalloc(size);
    
    format_rules = stack_create(sizeof(text_format), size/32);
    
    for (size_t i = 0; i < size; i++){
        char *instance = memmem(contents + i, size-i, "#include", 8);
        if (!instance) break;
        size_t new_i = instance-contents;
        i = new_i;
        stack_push(format_rules, &(text_format){ .color = 0xFFfcba03, .bounds = { i, 8 }});
    }
    
    code = (buffer){
        .buffer = contents, 
        .buffer_size = size,
        .cursor = 0,
        .limit = size,
        .options = buffer_can_grow,
    };
    
    draw_ctx ctx = {};
    request_draw_ctx(&ctx);
    
    sreadf("/theme", &palette, sizeof(palette));
    
    if (palette.bg) palette.bg -= 0x00111111;
    else if (!palette.fg) palette.fg = 0xFFFFFFFF;
    if (palette.fg) palette.fg -= 0x00111111;
    
    tf_info = (text_field_info){ &code, slice_from_literal(""), .multiline = true, .cursor_color = complementary_color(palette.bg)};
    
    set_document_view(ui, (gpu_rect){0,0,ctx.width,ctx.height});
    
    uno_focus(main_code);
    
    while (!should_close_ctx()){
        fb_clear(&ctx, palette.bg);
        uno_draw(&ctx);
        commit_draw_ctx(&ctx);
        kbd_event event = {};
        while (read_event(&event)){
            if (handle_modifier(&event)) continue;
            if (handle_copy(&event, uno_copy)) continue;
            if (handle_paste(&event, uno_paste)) continue;
            if (event.key == KEY_ESC) halt(0);
            else if (current_modifier & KEY_MOD_LMETA && event.type == KEY_PRESS && event.key == KEY_S){
                if (save_file())
                    print("Saved file");
            } else {
                uno_dispatch_kbd(event, current_modifier);
            }
        }
        mouse_data mouse = {};
        get_mouse_status(&mouse);
        uno_dispatch_mouse(mouse, current_modifier);    
    }
    
    return 0;
}