#include "syscalls/syscalls.h"
#include "files/helpers.h"
#include "files/buffer.h"
#include "input_keycodes.h"
#include "uno.h"
#include "math/math.h"
#include "ui/color/color.h"
#include "kbd_helper.h"
#include "memory/memory.h"
#include "header_utils/filebrowser.h"
#include "header_utils/composite.h"

enum { no_input, main_code } input_focus;

//TODO: file selector

struct {
    u32 bg;
    u32 fg;
} palette;

buffer code;

text_field_info tf_info;

string file_path;
string project_path;
string syntax_path;

arr_stack_t *format_rules;

string_slice file_relative_path(){
    if (file_path.data && project_path.data && file_path.length > project_path.length)
        return (string_slice){file_path.data+project_path.length,file_path.length-project_path.length};
    if (file_path.data) return slice_from_string(file_path);
    return (string_slice){};
}

void ui(){
    VERTICAL(((node_info){.sizing_rule = size_fill}),{
        uno_label((node_info){.sizing_rule = size_fit, .fg_color = 0xFF887766, .bg_color = 0x55554433 }, doc_text_footnote, file_relative_path());
        uno_text_field(main_code, (node_info){.sizing_rule = size_fill,.fg_color = palette.fg, .text_formatting = stack_to_text_format(format_rules) }, &tf_info);
    });
}

bool save_file(){
    if (!file_path.data) return false;
    return write_full_file(file_path.data, code.buffer, code.buffer_size);
}

void open_file(){
    size_t size = 0x1000;
    char *contents = read_full_file(file_path.data, &size);
    
    if (!contents) contents = zalloc(size);
    
    format_rules = stack_create(sizeof(text_format), size/32);

    if (syntax_path.data) string_free(syntax_path);

    syntax_path = string_format("/language/syntax/%v",file_relative_path());
    
    size_t format_size;
    void *syntax = read_full_file(syntax_path.data, &format_size);
    
    for (size_t s = 0; s < format_size; s += sizeof(text_format)){
        text_format *rule = (syntax + s);
        stack_push(format_rules, rule);
    }
    
    code = (buffer){
        .buffer = contents, 
        .buffer_size = size,
        .cursor = 0,
        .limit = size,
        .options = buffer_can_grow,
    };

    uno_refresh();
}

bool open_fb = false;
typedef enum { fb_file, fb_project } fb_behavior_e;

fb_behavior_e fb_behavior;

void toggle_fb(fb_behavior_e new_behavior){
    if (open_fb && fb_behavior == new_behavior) {
        open_fb = false;
        return;
    }
    open_fb = true;
    fb_behavior = new_behavior;
    switch (fb_behavior) {
        case fb_file:
            navigate(project_path.data);
            break;
        case fb_project:
            navigate("/home");
            break;
    }
}

bool handle_filepath(const char *name, const char *full_path){
    if (strend(name, ".red") == 0) return true;
    fs_stat stat = {};
    if (statf(full_path, &stat)){
        if ((fb_behavior == fb_file || !file_path.data) && stat.type == entry_file){
            fb_behavior = fb_file;
            file_path = string_from_literal(full_path);
            open_file();
            toggle_fb(fb_file);
            return true;
        }
        if (fb_behavior == fb_project && stat.type == entry_directory){
            if (current_modifier == KEY_MOD_LALT){
                project_path = string_from_literal(full_path);
                toggle_fb(fb_file);
                return true;
            }
        }
    } 
    return false;
}

int main(int argc, char *argv[]){
    
    if (argc){
        // -f name
        // -p name
        file_path = string_from_literal(argv[0]);
    }

    if (!file_path.data) file_path = string_from_literal("/home/projects/code/braincode/main.c");
    // if (!project_path.data) project_path = string_from_literal("/home/projects/code/braincode/");
    
    open_file();
    
    draw_ctx ctx = {};
    request_draw_ctx(&ctx);

    int fb_w = ctx.width/4, fb_h = ctx.height;
    
    filebrowser_ctx = dummy_draw_ctx(fb_w, fb_h);

    filebrowser_handle_path = handle_filepath;
    init_filebrowser(project_path.data);
    
    sreadf("/theme", &palette, sizeof(palette));
    
    if (palette.bg) palette.bg -= 0x00111111;
    else if (!palette.fg) palette.fg = 0xFFFFFFFF;
    if (palette.fg) palette.fg -= 0x00111111;
    
    tf_info = (text_field_info){ &code, slice_from_literal(""), .multiline = true, .cursor_color = complementary_color(palette.bg)};
    
    set_document_view(ui, (gpu_rect){0,0,ctx.width,ctx.height});
    
    uno_focus(main_code);

    if (!file_path.data){
        if (project_path.data)
            toggle_fb(fb_file);
        else toggle_fb(fb_project);
    }
    
    while (!should_close_ctx()){
        fb_clear(&ctx, palette.bg);
        uno_draw(&ctx);
        filebrowser_ctx.full_redraw = true;
        if (open_fb) composite(&filebrowser_ctx, (int_point){}, 1, &ctx);
        commit_draw_ctx(&ctx);
        kbd_event event = {};
        while (read_event(&event)){
            if (handle_modifier(&event)) continue;
            if (handle_copy(&event, uno_copy)) continue;
            if (handle_paste(&event, uno_paste)) continue;
            if (event.key == KEY_F1 && event.type == KEY_PRESS){
                toggle_fb(fb_file);
            }
            if (event.key == KEY_F2 && event.type == KEY_PRESS){
                toggle_fb(fb_project);
            }
            else if (open_fb) 
                filebrowser_input(event);
            else {
                if (event.key == KEY_ESC) halt(0);
                else if (current_modifier & KEY_MOD_LMETA && event.type == KEY_PRESS && event.key == KEY_S){
                    if (save_file())
                        print("Saved file");
                } else {
                    uno_dispatch_kbd(event, current_modifier);
                }
            }
        }
        mouse_data mouse = {};
        get_mouse_status(&mouse);
        uno_dispatch_mouse(mouse, current_modifier);    
    }
    
    return 0;
}