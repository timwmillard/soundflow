#include <stdio.h>
#include <pthread.h>

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"

#include "nuklear.h"
#include "sokol_nuklear.h"

#include "miniaudio.h"
#include "stb_image.h"

#include "gui.c"

#define WINDOW_WIDTH  1600
#define WINDOW_HEIGHT 1200

void update(double dt) 
{
    (void)dt;
}

void draw_ui(struct nk_context *ctx, int width, int height) 
{
    const int play_height = 45;
    play_controls(ctx, nk_rect(0, 0, width, play_height));
    node_editor(ctx, nk_rect(0, play_height, width, height - play_height));
}

void frame(void) 
{
    const double delta_time = sapp_frame_duration();
    const int width = sapp_width();
    const int height = sapp_height();

    update(delta_time);

    struct nk_context *ctx = snk_new_frame();
    draw_ui(ctx, width, height);


    // the sokol_gfx draw pass
    sg_begin_pass(&(sg_pass){
        .action = {
            .colors[0] = {
                .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.25f, 0.5f, 0.7f, 1.0f }
            }
        },
        .swapchain = sglue_swapchain()
    });
    snk_render(width, height);
    sg_end_pass();
    sg_commit();
}

#pragma clang diagnostic ignored "-Wswitch"
void input(const sapp_event* event)
{
    int files_dropped;
    const char *file_path;

    snk_handle_event(event);
    switch(event->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            if (event->key_code == SAPP_KEYCODE_ESCAPE) {
                sapp_request_quit();
            }
            break;
        case SAPP_EVENTTYPE_RESIZED:
            break;
        case SAPP_EVENTTYPE_FILES_DROPPED:
            files_dropped = sapp_get_num_dropped_files();
            for (int i=0; i<files_dropped; i++) {
                const char* file_path = sapp_get_dropped_file_path(i);
                node_editor_add_source_decoder(&nodeEditor, "Sound File",
                        nk_rect(event->mouse_x + (i * 10), event->mouse_y + (i * 10), 180, 220), 0, 1, file_path);
            }

            break;
    }
}

void init(void)
{
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    snk_setup(&(snk_desc_t){
        .enable_set_mouse_cursor = true,
        .dpi_scale = sapp_dpi_scale(),
        .logger.func = slog_func,
    });

    /*audio_init();*/
}

void cleanup(void)
{
    snk_shutdown();
    sg_shutdown();

    audio_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) 
{
    (void)argc;
    (void)argv;

    return (sapp_desc) {
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT,
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = input,
        .cleanup_cb = cleanup,
        .window_title = "Soundflow",
        .logger.func = slog_func,
        .enable_dragndrop = true,
        .high_dpi = false,
        .max_dropped_files = 20,
    };
}

