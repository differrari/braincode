#include "syscalls/syscalls.h"
#include "files/helpers.h"
#include "files/buffer.h"
#include "input_keycodes.h"
#include "uno.h"

enum { no_input, main_code } input_focus;

struct {
    u32 bg;
    u32 fg;
} palette;

buffer code;
gpu_point offset;

void ui(){
    uno_text_field(main_code, (node_info){.fg_color = palette.fg, .offset = offset, .sizing_rule = size_fill }, (text_field_info){&code, slice_from_literal(""),.multiline = true});
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
    
    palette.bg += 0x00111111;
    palette.fg -= 0x00111111;
    
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
                switch (event.key) {
                    case KEY_RIGHT: offset.x -= 20; break;
                    case KEY_LEFT: offset.x += 20; break;
                    case KEY_DOWN: offset.y -= 20; break;
                    case KEY_UP: offset.y += 20; break;
                    case KEY_PAGEDOWN: offset.y -= ctx.height; break;
                    case KEY_PAGEUP: offset.y += ctx.height; break;
                }
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
            offset.y += mouse.raw.scroll * 20;
            uno_refresh();
        }
    }
    
    return 0;
}