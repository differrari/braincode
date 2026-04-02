#include "syscalls/syscalls.h"
#include "files/helpers.h"
#include "files/buffer.h"
#include "input_keycodes.h"
#include "uno.h"
#include "math/math.h"
#include "ui/color/color.h"

enum { no_input, main_code } input_focus;

struct {
    u32 bg;
    u32 fg;
} palette;

buffer code;
gpu_point offset;

text_field_info tf_info;

void ui(){
    uno_text_field(main_code, (node_info){.fg_color = palette.fg, .offset = offset, .sizing_rule = size_fill }, &tf_info);
}

int main(int argc, char *argv[]){
    
    const char *file = "/shared/projects/code/braincode/main.c";
    
    size_t size;
    char *contents = read_full_file(file, &size);
    
    code = (buffer){
        .buffer = contents, 
        .buffer_size = size,
        .cursor = 0,
        .limit = size,
        .options = buffer_can_grow,
    };
    
    print(contents);
    
    draw_ctx ctx = {};
    request_draw_ctx(&ctx);
    
    sreadf("/theme", &palette, sizeof(palette));
    
    palette.bg -= 0x00111111;
    palette.fg -= 0x00111111;
    
    tf_info = (text_field_info){ &code, slice_from_literal(""), .multiline = true, .cursor_color = complementary_color(palette.bg)};
    
    set_document_view(ui, (gpu_rect){0,0,ctx.width,ctx.height});
    
    uno_focus(main_code);
    
    while (!should_close_ctx()){
        fb_clear(&ctx, palette.bg);
        uno_draw(&ctx);
        commit_draw_ctx(&ctx);
        kbd_event event = {};
        if (read_event(&event)){
            if (event.key == KEY_ESC) halt(0);
            else if (event.type == KEY_PRESS && ((event.key >= KEY_RIGHT && event.key <= KEY_UP) || event.key == KEY_PAGEUP || event.key == KEY_PAGEDOWN )){
                i32 x_shift, y_shift = 0;
                switch (event.key) {
                    case KEY_RIGHT: x_shift = 1; break;
                    case KEY_LEFT: x_shift = -1; break;
                    case KEY_DOWN: y_shift = 1; break;
                    case KEY_UP: y_shift = -1; break;
                    case KEY_PAGEDOWN: offset.y -= ctx.height; break;
                    case KEY_PAGEUP: offset.y += ctx.height; break;
                }
                if (x_shift)
                    code.cursor += x_shift;
                if (y_shift){
                    string_slice slice = (string_slice){code.buffer,code.buffer_size};
                    i32 lin, col = 0;
                    pos_to_lin_col(code.cursor, slice, &lin, &col);
                    lin += y_shift;
                    code.cursor = lin_col_to_pos(lin, col, slice);
                }
                code.cursor = clamp(code.cursor, 0, code.buffer_size-1);
                uno_refresh();
            }
            else {
                uno_dispatch_kbd(event);
            }
        }
        mouse_data mouse = {};
        get_mouse_status(&mouse);
        uno_dispatch_mouse(mouse);    
        if (mouse.raw.scroll){
            if (tf_info.modifier & KEY_MOD_LSHIFT)
                offset.x += mouse.raw.scroll * 20;
            else        
                offset.y += mouse.raw.scroll * 20;
            uno_refresh();
        }
    }
    
    return 0;
}