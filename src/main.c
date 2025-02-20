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

static struct {
    /*sg_pipeline pip;*/
    /*sg_bindings bind;*/
    /*sg_pass_action pass_action;*/
    /*sg_buffer vbuf;*/
    ma_device audio_device;
    ma_node_graph node_graph;
    ma_waveform sine_wave;
} state;



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

#define CHANNELS 2
#define FORMAT ma_format_f32
#define SAMPLE_RATE 48000

void playback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    (void)pInput;
    assert(pDevice->playback.channels == CHANNELS);

    ma_node_graph_read_pcm_frames(&state.node_graph, pOutput, frameCount, NULL);
}

void audio_init(void)
{
    ma_result result;

    ma_node_graph_config node_graph_config = ma_node_graph_config_init(CHANNELS);
    result = ma_node_graph_init(&node_graph_config, NULL, &state.node_graph);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Error: failed to init node graph, error code = %d\n", result);
        exit(1);
    }

    // Decoder
    ma_decoder decoder;
    ma_decoder_config decoder_config = ma_decoder_config_init(FORMAT, CHANNELS, SAMPLE_RATE);
    result = ma_decoder_init_file("sounds/jungle.wav", &decoder_config, &decoder);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Error: failed to initalise decoder, error code = %d\n", result);
        exit(1);
    }

    // Device Setup
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = FORMAT;
    config.playback.channels = CHANNELS;
    config.sampleRate = SAMPLE_RATE;
    config.dataCallback = playback;

    // Data Source
    ma_data_source_node source_node;
    ma_data_source_node_config source_node_config = ma_data_source_node_config_init(&decoder);
    result = ma_data_source_node_init(&state.node_graph, &source_node_config, NULL, &source_node);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Error: failed to initalise source node, error code = %d\n", result);
        exit(1);
    }

    ma_node_attach_output_bus(&source_node, 0, NULL, 0);
    /*MA_API ma_result ma_node_attach_output_bus(ma_node* pNode, ma_uint32 outputBusIndex, ma_node* pOtherNode, ma_uint32 otherNodeInputBusIndex)*/
    /*ma_node_attach_output_bus(&g_pSoundNodes[g_soundNodeCount].node, 0, &g_splitterNode, 0);*/

    result = ma_device_init(NULL, &config, &state.audio_device);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Error: failed to initalise device, error code = %d\n", result);
        exit(1);
    }

    result = ma_device_start(&state.audio_device);
    if (result != MA_SUCCESS) {
        // Handle error
        ma_device_uninit(&state.audio_device);
        fprintf(stderr, "Error: failed to start device, error code = %d\n", result);
        exit(1);
    }
    printf("init audio subsystem\n");
}

void audio_shutdown(void)
{
    ma_device_uninit(&state.audio_device);
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

    audio_init();
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
        .logger.func = slog_func
    };
}

