
#include <stdio.h>

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"

#include "nuklear.h"
#include "sokol_nuklear.h"

#include "shader_glsl.h"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    sg_buffer vbuf;
} state;



void update(float dt) {
    (void)dt;
}

void frame(void) {

    const float delta_time = (float)sapp_frame_duration();
    const float w = sapp_widthf();
    const float h = sapp_heightf();

    update(delta_time);

}

#pragma clang diagnostic ignored "-Wswitch"
void input(const sapp_event* event) {
    switch(event->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            if (event->key_code == SAPP_KEYCODE_ESCAPE) {
                sapp_request_quit();
            }
            break;
    }
}

void init(void) {

    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    snk_setup(&(snk_desc_t){
        .enable_set_mouse_cursor = true,
        .dpi_scale = sapp_dpi_scale(),
        .logger.func = slog_func,
    });
}

void cleanup(void) {
    snk_shutdown();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return (sapp_desc) {
        .width = 640,
        .height = 480,
        .init_cb = init,
        .frame_cb = frame,
        .event_cb = input,
        .cleanup_cb = cleanup,
        .window_title = "Soundflow",
        .logger.func = slog_func
    };
}

