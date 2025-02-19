#include <stdio.h>
#include <pthread.h>

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"

#include "nuklear.h"
#include "sokol_nuklear.h"

#include "miniaudio.h"

#include "node_editor.c"

/*#define STB_IMAGE_IMPLEMENTATION*/
#include "stb_image.h"

#define WINDOW_WIDTH  1600
#define WINDOW_HEIGHT 1200

/*static struct {*/
/*    sg_pipeline pip;*/
/*    sg_bindings bind;*/
/*    sg_pass_action pass_action;*/
/*    sg_buffer vbuf;*/
/*} state;*/



void update(double dt) 
{
    (void)dt;
}

void draw_ui(struct nk_context *ctx, int width, int height) 
{
    node_editor(ctx, width, height);
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
    snk_handle_event(event);
    switch(event->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            if (event->key_code == SAPP_KEYCODE_ESCAPE) {
                sapp_request_quit();
            }
            break;
        case SAPP_EVENTTYPE_RESIZED:
            break;
    }
}

void playback_beep(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    (void)pInput;
    ma_waveform *sine_wave = pDevice->pUserData;
    assert(sine_wave != NULL);

    ma_uint64 framesRead;
    ma_waveform_read_pcm_frames(sine_wave, pOutput, frameCount, &framesRead);
}

void init_audio(void)
{
    printf("init audio subsystem\n");
    ma_result result;

    // Sine wave generator
    ma_waveform sine_wave;
    ma_waveform_config sine_config = ma_waveform_config_init(ma_format_s16, 2, 44100, ma_waveform_type_sine, 0.8, 440);
    ma_waveform_init(&sine_config, &sine_wave);

    // Setup Miniaudio
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = sine_config.format;
    config.playback.channels = sine_config.channels;
    config.sampleRate = sine_config.sampleRate;
    config.dataCallback = playback_beep;
    config.pUserData = &sine_wave;

    ma_device device;
    result = ma_device_init(NULL, &config, &device);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Error: failed to initalise device, error code = %d\n", result);
        exit(1);
    }

    result = ma_device_start(&device);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Error: failed to start device, error code = %d\n", result);
        exit(1);
    }

    getchar();
}

void *audio_thread(void *data)
{
    (void)data;
    printf("init audio thread\n");
    init_audio();
    return NULL;
}


void init(void)
{
    pthread_t th;
    if (pthread_create(&th, NULL, audio_thread, NULL)) {
        fprintf(stderr, "Error: init audio thread\n");
        exit(1);
    }
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    snk_setup(&(snk_desc_t){
        .enable_set_mouse_cursor = true,
        .dpi_scale = sapp_dpi_scale(),
        .logger.func = slog_func,
    });


    /*init_audio();*/
}

void cleanup(void)
{
    snk_shutdown();
    sg_shutdown();

    /*ma_device_uninit(&device);*/
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
        .logger.func = slog_func
    };
}

