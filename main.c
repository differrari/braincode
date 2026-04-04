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

text_field_info tf_info;

const char *file_path = "/shared/projects/code/braincode/main.c";

void ui(){
    HORIZONTAL(((node_info){.sizing_rule = size_fill, .bg_color = 0xFF126100}),{
        uno_text_field(main_code, (node_info){.sizing_rule = size_fill,.fg_color = palette.fg }, &tf_info);
        VERTICAL(((node_info){.bg_color = 0xFF664b00, .sizing_rule = size_relative, .percentage = 0.25f, .padding = 10 }), {
            uno_label((node_info){}, doc_text_body, slice_from_literal("file1"));
            uno_label((node_info){}, doc_text_body, slice_from_literal("file2"));
            uno_label((node_info){}, doc_text_body, slice_from_literal("file3"));
        });
    });
}

void scroll_in_line(bool begin){
    bool found = false;
    if (begin){
        for (u64 i = code.cursor; i > 0; i--)
            if (((char*)code.buffer)[i-1] == '\n'){ found = true; code.cursor = i; break; }
        if (!found) code.cursor = 0;
    } else {
        for (u64 i = code.cursor; i < code.buffer_size; i++)
            if (((char*)code.buffer)[i+1] == '\n'){ found = true; code.cursor = i; break; }
        if (!found) code.cursor = code.buffer_size;
    }
    code.cursor = clamp(code.cursor, 0, code.buffer_size);
    uno_refresh();
}

bool save_file(){
    return write_full_file(file_path, code.buffer, code.buffer_size);
}

int main(int argc, char *argv[]){
    
    size_t size = 0x1000;
    char *contents = read_full_file(file_path, &size);
    
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
            if (event.key == KEY_HOME)
                scroll_in_line(true);
            if (event.key == KEY_END)
                scroll_in_line(false);
            else if (event.type == KEY_PRESS && ((event.key >= KEY_RIGHT && event.key <= KEY_UP) || event.key == KEY_PAGEUP || event.key == KEY_PAGEDOWN )){
                i32 x_shift = 0;
                i32 y_shift = 0;
                switch (event.key) {
                    case KEY_RIGHT: x_shift = 1; break;
                    case KEY_LEFT:  x_shift = -1; break;
                    case KEY_DOWN:  y_shift = 1; break;
                    case KEY_UP:    y_shift = -1; break;
                    case KEY_PAGEDOWN:  
                        // y_shift = (ctx.height/line_height)-1;//DEADLINE: 2026-06-09 patent on PGUP/PGDOWN expires
                        // offset.y -= ctx.height;
                        break;
                    case KEY_PAGEUP: 
                        // y_shift = -(ctx.height/line_height)-1;//DEADLINE: 2026-06-09 patent on PGUP/PGDOWN expires
                        // offset.y += ctx.height;
                        break;
                }
                if (x_shift || y_shift)
                    uno_text_field_shift_cursor(main_code, x_shift,y_shift);
                uno_refresh();
            } else if (tf_info.modifier == KEY_MOD_LCTRL && event.type == KEY_PRESS && event.key == KEY_S){
                if (save_file())
                    print("Saved file");
            } else {
                uno_dispatch_kbd(event);
            }
        }
        mouse_data mouse = {};
        get_mouse_status(&mouse);
        uno_dispatch_mouse(mouse);    
        if (mouse.raw.scroll){
            if (tf_info.modifier & KEY_MOD_LSHIFT){
                uno_text_field_scroll(main_code, mouse.raw.scroll, 0);
            } else {        
                uno_text_field_scroll(main_code, 0, mouse.raw.scroll);
            }
            uno_refresh();
        }
    }
    
    return 0;
}