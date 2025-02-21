/* nuklear - v1.00 - public domain */
/* This is a simple node editor just to show a simple implementation and that
 * it is possible to achieve it with this library. While all nodes inside this
 * example use a simple color modifier as content you could change them
 * to have your custom content depending on the node time.
 * Biggest difference to most usual implementation is that this example does
 * not have connectors on the right position of the property that it links.
 * This is mainly done out of laziness and could be implemented as well but
 * requires calculating the position of all rows and add connectors.
 * In addition adding and removing nodes is quite limited at the
 * moment since it is based on a simple fixed array. If this is to be converted
 * into something more serious it is probably best to extend it.*/

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "nuklear.h"
#include "miniaudio.h"

#define CHANNELS 2
#define FORMAT ma_format_f32
#define SAMPLE_RATE 48000

static struct {
    ma_device device;
} audio_state;

enum node_tag {
    NODE_COLOR,
    NODE_SOURCE_SOUND,
    NODE_ENDPOINT,
    NODE_SOURCE_DECODER,
};

struct node_endpoint {
    ma_node *endpoint;
};

struct node_source_decoder {
    ma_decoder decoder;
    ma_data_source_node source;
    char file_name[32];
};

struct node_color {
    float value;
    struct nk_color color;
};

struct node_source_sound {
    char file_name[32];
    int volume;
};

struct node {
    enum node_tag tag;
    int ID;
    char name[32];
    struct nk_rect bounds;
    int input_count;
    int output_count;
    struct node *next;
    struct node *prev;

    ma_node *audio_node;

    union {
        struct node_color color;
        struct node_source_sound source_sound;
        struct node_endpoint endpoint;
        struct node_source_decoder source_decoder;
    };
};

struct node_link {
    int input_id;
    int input_slot;
    int output_id;
    int output_slot;
    /*struct nk_vec2 in;*/
    /*struct nk_vec2 out;*/
};

struct node_linking {
    int active;
    struct node *node;
    int input_id;
    int input_slot;
};

struct node_editor {
    int initialized;
    struct node node_buf[32];
    struct node_link links[64];
    struct node *begin;
    struct node *end;
    int node_count;
    int link_count;
    struct nk_rect bounds;
    struct node *selected;
    int show_grid;
    struct nk_vec2 scrolling;
    struct node_linking linking;
    ma_node_graph audio_graph;
};
static struct node_editor nodeEditor;

void playback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    (void)pInput;
    assert(pDevice->playback.channels == CHANNELS);

    ma_node_graph_read_pcm_frames(&nodeEditor.audio_graph, pOutput, frameCount, NULL);
}

void audio_init(void)
{
    ma_result result;

    /*ma_node_graph_config node_graph_config = ma_node_graph_config_init(CHANNELS);*/
    /*result = ma_node_graph_init(&node_graph_config, NULL, &audio_state.node_graph);*/
    /*if (result != MA_SUCCESS) {*/
    /*    fprintf(stderr, "Error: failed to init node graph, error code = %d\n", result);*/
    /*    exit(1);*/
    /*}*/

    /*// Decoder*/
    /*ma_decoder_config decoder_config = ma_decoder_config_init(FORMAT, CHANNELS, SAMPLE_RATE);*/
    /*result = ma_decoder_init_file("sounds/jungle.mp3", &decoder_config, &audio_state.decoder);*/
    /*if (result != MA_SUCCESS) {*/
    /*    fprintf(stderr, "Error: failed to initalise decoder, error code = %d\n", result);*/
    /*    exit(1);*/
    /*}*/

    /*// Data Source*/
    /*ma_data_source_node_config source_node_config = ma_data_source_node_config_init(&audio_state.decoder);*/
    /*result = ma_data_source_node_init(&audio_state.node_graph, &source_node_config, NULL, &audio_state.source_node);*/
    /*if (result != MA_SUCCESS) {*/
    /*    fprintf(stderr, "Error: failed to initalise source node, error code = %d\n", result);*/
    /*    exit(1);*/
    /*}*/

    /*ma_node_attach_output_bus(&audio_state.source_node, 0, ma_node_graph_get_endpoint(&audio_state.node_graph), 0);*/

    // Device Setup
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = FORMAT;
    config.playback.channels = CHANNELS;
    config.sampleRate = SAMPLE_RATE;
    config.dataCallback = playback;
    result = ma_device_init(NULL, &config, &audio_state.device);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Error: failed to initalise device, error code = %d\n", result);
        exit(1);
    }

    result = ma_device_start(&audio_state.device);
    if (result != MA_SUCCESS) {
        // Handle error
        ma_device_uninit(&audio_state.device);
        fprintf(stderr, "Error: failed to start device, error code = %d\n", result);
        exit(1);
    }
    printf("init audio subsystem\n");
}

void audio_shutdown(void)
{
    ma_device_uninit(&audio_state.device);
}

static void
node_editor_push(struct node_editor *editor, struct node *node)
{
    if (!editor->begin) {
        node->next = NULL;
        node->prev = NULL;
        editor->begin = node;
        editor->end = node;
    } else {
        node->prev = editor->end;
        if (editor->end)
            editor->end->next = node;
        node->next = NULL;
        editor->end = node;
    }
}

static void
node_editor_pop(struct node_editor *editor, struct node *node)
{
    if (node->next)
        node->next->prev = node->prev;
    if (node->prev)
        node->prev->next = node->next;
    if (editor->end == node)
        editor->end = node->prev;
    if (editor->begin == node)
        editor->begin = node->next;
    node->next = NULL;
    node->prev = NULL;
}

static struct node*
node_editor_find(struct node_editor *editor, int ID)
{
    struct node *iter = editor->begin;
    while (iter) {
        if (iter->ID == ID)
            return iter;
        iter = iter->next;
    }
    return NULL;
}

static struct node*
node_editor_add(struct node_editor *editor, const char *name, struct nk_rect bounds,
     int in_count, int out_count)
{
    static int IDs = 0;
    struct node *node;
    assert((nk_size)editor->node_count < NK_LEN(editor->node_buf));
    node = &editor->node_buf[editor->node_count++];
    node->ID = IDs++;
    node->input_count = in_count;
    node->output_count = out_count;
    node->bounds = bounds;
    strcpy(node->name, name);
    node->audio_node = NULL;

    node_editor_push(editor, node);
    return node;
}

static struct node* node_editor_node_by_id(struct node_editor *editor, const int id)
{
    for (int i=0; i<editor->node_count; i++) {
        struct node node = editor->node_buf[i];
        if (node.ID == id) return &editor->node_buf[i];
    }
    return NULL;
}

static void
node_editor_add_color(struct node_editor *editor, const char *name, struct nk_rect bounds,
    int in_count, int out_count,
    struct nk_color col )
{
    struct node *node = node_editor_add(editor, name, bounds, in_count, out_count);
    node->tag = NODE_COLOR;
    node->color.value = 0;
    node->color.color = col;
}

static void
node_editor_add_source_sound(struct node_editor *editor, const char *name, struct nk_rect bounds,
    int in_count, int out_count,
    char *file_name )
{
    struct node *node = node_editor_add(editor, name, bounds, in_count, out_count);
    node->tag = NODE_SOURCE_SOUND;
    strcpy(node->source_sound.file_name, file_name);
    node->source_sound.volume = 5;
}

// Endpoint
static void
node_editor_add_endpoint(struct node_editor *editor, const char *name, struct nk_rect bounds,
    int in_count, int out_count)
{
    struct node *node = node_editor_add(editor, name, bounds, in_count, out_count);
    node->tag = NODE_ENDPOINT;

    ma_node *endpoint = ma_node_graph_get_endpoint(&editor->audio_graph);
    node->endpoint.endpoint = endpoint;
    node->audio_node = endpoint;
}

// Source Decoder
static void
node_editor_add_source_decoder(struct node_editor *editor, const char *name, struct nk_rect bounds,
    int in_count, int out_count)
{
    struct node *node = node_editor_add(editor, name, bounds, in_count, out_count);
    node->tag = NODE_SOURCE_DECODER;

    strcpy(node->source_decoder.file_name, "sounds/jungle.mp3");
    ma_result result;

    // Decoder
    ma_decoder_config decoder_config = ma_decoder_config_init(FORMAT, CHANNELS, SAMPLE_RATE);
    /*result = ma_decoder_init_file(node->source_decoder.file_name, &decoder_config, &node->source_decoder.decoder);*/
    result = ma_decoder_init_file("sounds/jungle.mp3", &decoder_config, &node->source_decoder.decoder);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Error: failed to initalise decoder, error code = %d\n", result);
        exit(1);
    }

    // Data Source
    ma_data_source_node_config source_node_config = ma_data_source_node_config_init(&node->source_decoder.decoder);
    result = ma_data_source_node_init(&editor->audio_graph, &source_node_config, NULL, &node->source_decoder.source);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Error: failed to initalise source node, error code = %d\n", result);
        exit(1);
    }
    node->audio_node = &node->source_decoder.source;
}

static void
node_editor_link(struct node_editor *editor, int in_id, int in_slot,
    int out_id, int out_slot)
{
    struct node_link *link;
    assert((nk_size)editor->link_count < NK_LEN(editor->links));
    link = &editor->links[editor->link_count++];
    link->input_id = in_id;
    link->input_slot = in_slot;
    link->output_id = out_id;
    link->output_slot = out_slot;

    // out -> in
    struct node *out_node = node_editor_node_by_id(editor, out_id); 
    struct node *in_node = node_editor_node_by_id(editor, in_id);
    if (out_node == NULL || in_node == NULL 
            || out_node->audio_node == NULL || in_node->audio_node == NULL)
            return;

    printf("[DEBUG] connecting nodes %d(%d) -> %d(%d)\n", out_node->ID, out_slot, in_node->ID, in_slot);
    ma_result result = ma_node_attach_output_bus(in_node->audio_node, in_slot, out_node->audio_node, out_slot);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "[ERROR]: failed to link nodes, error code = %d\n", result);
    }
}

// TODO: this is broken
static void
node_editor_unlink_in(struct node_editor *editor, int in_id, int in_slot)
{
    struct node_link *link;
    nk_bool move = nk_false;
    for (int i=0; i<editor->link_count; i++) {
        link = &editor->links[i];
        if (!move && (link->input_id == in_id && link->input_slot == in_slot))
            move = nk_true;

        if (move) {
            if (i < editor->link_count - 1)
                editor->links[i] = editor->links[i+1];
        }
    }
    editor->link_count--;
}

// TODO: this is broken
static void
node_editor_unlink_out(struct node_editor *editor, int out_id, int out_slot)
{
    struct node_link *link;
    nk_bool move = nk_false;
    for (int i=0; i<editor->link_count; i++) {
        link = &editor->links[i];
        if (!move && (link->output_id == out_id && link->output_slot == out_slot))
            move = nk_true;

        if (move) {
            if (i < editor->link_count - 1)
                editor->links[i] = editor->links[i+1];
        }
    }
    editor->link_count--;
}

static void
node_editor_init(struct node_editor *editor)
{
    audio_init();

    memset(editor, 0, sizeof(*editor));

    ma_node_graph_config node_graph_config = ma_node_graph_config_init(CHANNELS);
    ma_result result = ma_node_graph_init(&node_graph_config, NULL, &editor->audio_graph);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Error: failed to init node graph, error code = %d\n", result);
        exit(1);
    }

    /*node_editor_add_source_sound(editor, "Data Source 1", nk_rect(40, 10, 180, 220), 0, 1, "my_music.mp3");*/
    /*node_editor_add_source_sound(editor, "Data Source 2", nk_rect(40, 260, 180, 220), 0, 1, "my_music.mp3");*/
    /*node_editor_add_source_sound(editor, "Splitter", nk_rect(300, 200, 180, 220), 1, 2, "");*/
    /*node_editor_add_source_sound(editor, "Low Pass Filter", nk_rect(600, 100, 180, 220), 1, 1, "");*/
    /*node_editor_add_source_sound(editor, "Echo / Delay", nk_rect(600, 400, 180, 220), 1, 1, "");*/
    /*node_editor_add_source_sound(editor, "End Point", nk_rect(900, 200, 180, 220), 1, 0, "");*/
    /*node_editor_link(editor, 0, 0, 2, 0);*/
    /*node_editor_link(editor, 1, 0, 2, 0);*/
    /*node_editor_link(editor, 2, 0, 3, 0);*/
    /*node_editor_link(editor, 2, 1, 4, 0);*/
    /*node_editor_link(editor, 3, 0, 5, 0);*/
    /*node_editor_link(editor, 4, 0, 5, 0);*/

    node_editor_add_source_decoder(editor, "Data Source 1", nk_rect(40, 10, 180, 220), 0, 1);
    node_editor_add_endpoint(editor, "Endpoint", nk_rect(540, 10, 180, 220), 1, 0);
    node_editor_link(editor, 0, 0, 1, 0);

    editor->show_grid = nk_true;
}

static void play_controls(struct nk_context *ctx, struct nk_rect bounds)
{
    float value;
    int op = 1;

    if (nk_begin(ctx, "Controls", bounds,
                NK_WINDOW_BORDER|NK_WINDOW_NO_SCROLLBAR)) {
        nk_layout_row_dynamic(ctx, 30, 20);
        nk_spacing(ctx, 6);
        if (nk_button_label(ctx, "Play")) {
            ma_device_start(&audio_state.device);
        }
        if (nk_button_label(ctx, "Stop")) {
            ma_device_stop(&audio_state.device);
        }
    }
    nk_end(ctx);
}


static int node_editor(struct nk_context *ctx, struct nk_rect bounds)
{
    int n = 0;
    struct nk_rect total_space;
    const struct nk_input *in = &ctx->input;
    struct nk_command_buffer *canvas;
    struct node *updated = 0;
    struct node_editor *nodedit = &nodeEditor;

    if (!nodeEditor.initialized) {
        node_editor_init(&nodeEditor);
        nodeEditor.initialized = 1;
    }

    if (nk_begin(ctx, "Node Editor", bounds,
        NK_WINDOW_BORDER|NK_WINDOW_NO_SCROLLBAR)) {

        /* allocate complete window space */
        canvas = nk_window_get_canvas(ctx);
        total_space = nk_window_get_content_region(ctx);
        nk_layout_space_begin(ctx, NK_STATIC, total_space.h, nodedit->node_count);
        {
            struct node *it = nodedit->begin;
            struct nk_rect size = nk_layout_space_bounds(ctx);
            struct nk_panel *node = 0;

            if (nodedit->show_grid) {
                /* display grid */
                float x, y;
                const float grid_size = 32.0f;
                const struct nk_color grid_color = nk_rgb(50, 50, 50);
                for (x = (float)fmod(size.x - nodedit->scrolling.x, grid_size); x < size.w; x += grid_size)
                    nk_stroke_line(canvas, x+size.x, size.y, x+size.x, size.y+size.h, 1.0f, grid_color);
                for (y = (float)fmod(size.y - nodedit->scrolling.y, grid_size); y < size.h; y += grid_size)
                    nk_stroke_line(canvas, size.x, y+size.y, size.x+size.w, y+size.y, 1.0f, grid_color);
            }

            /* execute each node as a movable group */
            while (it) {
                /* calculate scrolled node window position and size */
                nk_layout_space_push(ctx, nk_rect(it->bounds.x - nodedit->scrolling.x,
                    it->bounds.y - nodedit->scrolling.y, it->bounds.w, it->bounds.h));

                /* execute node window */
                if (nk_group_begin(ctx, it->name, NK_WINDOW_MOVABLE|NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BORDER|NK_WINDOW_TITLE))
                {
                    /* always have last selected node on top */

                    node = nk_window_get_panel(ctx);
                    if (nk_input_mouse_clicked(in, NK_BUTTON_LEFT, node->bounds) &&
                        (!(it->prev && nk_input_mouse_clicked(in, NK_BUTTON_LEFT,
                        nk_layout_space_rect_to_screen(ctx, node->bounds)))) &&
                        nodedit->end != it)
                    {
                        updated = it;
                    }

                    /* ================= NODE CONTENT =====================*/
                    nk_layout_row_dynamic(ctx, 25, 1);
                    #pragma clang diagnostic ignored "-Wswitch"
                    switch (it->tag) {
                        case NODE_COLOR:
                            nk_button_color(ctx, it->color.color);
                            it->color.color.r = (nk_byte)nk_propertyi(ctx, "#R:", 0, it->color.color.r, 255, 1,1);
                            it->color.color.g = (nk_byte)nk_propertyi(ctx, "#G:", 0, it->color.color.g, 255, 1,1);
                            it->color.color.b = (nk_byte)nk_propertyi(ctx, "#B:", 0, it->color.color.b, 255, 1,1);
                            it->color.color.a = (nk_byte)nk_propertyi(ctx, "#A:", 0, it->color.color.a, 255, 1,1);
                            break;
                        case NODE_SOURCE_SOUND:
                            nk_label(ctx, it->source_sound.file_name, NK_TEXT_ALIGN_CENTERED);
                            nk_property_int(ctx, "#Volume", 0, &it->source_sound.volume, 20, 1, 0.5);
                            break;
                        case NODE_ENDPOINT:
                            nk_label(ctx, "Audio Device", NK_TEXT_ALIGN_CENTERED);
                            break;
                        case NODE_SOURCE_DECODER:
                            nk_label(ctx, it->source_decoder.file_name, NK_TEXT_ALIGN_CENTERED);
                            break;
                    }
                    /* ====================================================*/
                    nk_group_end(ctx);
                }
                {
                    /* node connector and linking */
                    float space;
                    struct nk_rect bounds;
                    bounds = nk_layout_space_rect_to_local(ctx, node->bounds);
                    bounds.x += nodedit->scrolling.x;
                    bounds.y += nodedit->scrolling.y;
                    it->bounds = bounds;

                    /* output connector */
                    space = node->bounds.h / (float)((it->output_count) + 1);
                    for (n = 0; n < it->output_count; ++n) {
                        struct nk_rect circle;
                        circle.x = node->bounds.x + node->bounds.w-4;
                        circle.y = node->bounds.y + space * (float)(n+1);
                        circle.w = 8; circle.h = 8;
                        struct nk_color cir_color = nk_rgb(100, 100, 100);
                        if (nk_input_is_mouse_hovering_rect(in, circle) || 
                            (nodedit->linking.active && nodedit->linking.node == it &&
                            nodedit->linking.input_slot == n))
                            cir_color = nk_rgb(100, 200, 100);
                        nk_fill_circle(canvas, circle, cir_color);

                        /* delete link */
                        if (nk_input_has_mouse_click_down_in_rect(in, NK_BUTTON_RIGHT, circle, nk_true)) {
                            node_editor_unlink_out(nodedit, it->ID, n);
                        }

                        /* start linking process */
                        if (nk_input_has_mouse_click_down_in_rect(in, NK_BUTTON_LEFT, circle, nk_true)) {
                            nodedit->linking.active = nk_true;
                            nodedit->linking.node = it;
                            nodedit->linking.input_id = it->ID;
                            nodedit->linking.input_slot = n;
                        }

                        /* draw curve from linked node slot to mouse position */
                        if (nodedit->linking.active && nodedit->linking.node == it &&
                            nodedit->linking.input_slot == n) {
                            struct nk_vec2 l0 = nk_vec2(circle.x + 3, circle.y + 3);
                            struct nk_vec2 l1 = in->mouse.pos;
                            nk_stroke_curve(canvas, l0.x, l0.y, l0.x + 50.0f, l0.y,
                                l1.x - 50.0f, l1.y, l1.x, l1.y, 1.0f, nk_rgb(100, 100, 100));
                        }
                    }

                    /* input connector */
                    space = node->bounds.h / (float)((it->input_count) + 1);
                    for (n = 0; n < it->input_count; ++n) {
                        struct nk_rect circle;
                        circle.x = node->bounds.x-4;
                        circle.y = node->bounds.y + space * (float)(n+1);
                        circle.w = 8; circle.h = 8;
                        struct nk_color cir_color = nk_rgb(100, 100, 100);
                        if (nk_input_is_mouse_hovering_rect(in, circle) && 
                            nodedit->linking.active)
                            cir_color = nk_rgb(100, 200, 100);
                        nk_fill_circle(canvas, circle, cir_color);

                        /* delete link */
                        if (nk_input_has_mouse_click_down_in_rect(in, NK_BUTTON_RIGHT, circle, nk_true)) {
                            node_editor_unlink_in(nodedit, it->ID, n);
                        }

                        if (nk_input_is_mouse_released(in, NK_BUTTON_LEFT) &&
                            nk_input_is_mouse_hovering_rect(in, circle) &&
                            nodedit->linking.active && nodedit->linking.node != it) {
                            nodedit->linking.active = nk_false;
                            node_editor_link(nodedit, nodedit->linking.input_id,
                                nodedit->linking.input_slot, it->ID, n);
                        }
                    }
                }
                it = it->next;
            }

            /* reset linking connection */
            if (nodedit->linking.active && nk_input_is_mouse_released(in, NK_BUTTON_LEFT)) {
                nodedit->linking.active = nk_false;
                nodedit->linking.node = NULL;
                fprintf(stdout, "linking failed\n");
            }

            /* draw each link */
            for (n = 0; n < nodedit->link_count; ++n) {
                struct node_link *link = &nodedit->links[n];
                struct node *ni = node_editor_find(nodedit, link->input_id);
                struct node *no = node_editor_find(nodedit, link->output_id);
                float spacei = node->bounds.h / (float)((ni->output_count) + 1);
                float spaceo = node->bounds.h / (float)((no->input_count) + 1);
                struct nk_vec2 l0 = nk_layout_space_to_screen(ctx,
                    nk_vec2(ni->bounds.x + ni->bounds.w, 3.0f + ni->bounds.y + spacei * (float)(link->input_slot+1)));
                struct nk_vec2 l1 = nk_layout_space_to_screen(ctx,
                    nk_vec2(no->bounds.x, 3.0f + no->bounds.y + spaceo * (float)(link->output_slot+1)));

                l0.x -= nodedit->scrolling.x;
                l0.y -= nodedit->scrolling.y;
                l1.x -= nodedit->scrolling.x;
                l1.y -= nodedit->scrolling.y;
                nk_stroke_curve(canvas, l0.x, l0.y, l0.x + 50.0f, l0.y,
                    l1.x - 50.0f, l1.y, l1.x, l1.y, 1.0f, nk_rgb(100, 100, 100));
            }

            if (updated) {
                /* reshuffle nodes to have least recently selected node on top */
                node_editor_pop(nodedit, updated);
                node_editor_push(nodedit, updated);
            }

            /* node selection */
            if (nk_input_mouse_clicked(in, NK_BUTTON_LEFT, nk_layout_space_bounds(ctx))) {
                it = nodedit->begin;
                nodedit->selected = NULL;
                nodedit->bounds = nk_rect(in->mouse.pos.x, in->mouse.pos.y, 100, 200);
                while (it) {
                    struct nk_rect b = nk_layout_space_rect_to_screen(ctx, it->bounds);
                    b.x -= nodedit->scrolling.x;
                    b.y -= nodedit->scrolling.y;
                    if (nk_input_is_mouse_hovering_rect(in, b))
                        nodedit->selected = it;
                    it = it->next;
                }
            }

            /* contextual menu */
            if (nk_contextual_begin(ctx, 0, nk_vec2(140, 220), nk_window_get_bounds(ctx))) {
                const char *grid_option[] = {"Show Grid", "Hide Grid"};
                nk_layout_row_dynamic(ctx, 25, 1);
                if (nk_contextual_item_label(ctx, "New Audio File", NK_TEXT_CENTERED))
                    node_editor_add_source_decoder(nodedit, "Decoder", nk_rect(400, 260, 180, 220),
                             0, 2);
                if (nk_contextual_item_label(ctx, "New Endpoint", NK_TEXT_CENTERED))
                    node_editor_add_endpoint(nodedit, "Endpoint", nk_rect(400, 260, 180, 220),
                             1, 0);
                /*if (nk_contextual_item_label(ctx, "New Sound", NK_TEXT_CENTERED))*/
                /*    node_editor_add_source_sound(nodedit, "Souce Sound", nk_rect(400, 260, 180, 220),*/
                /*             1, 2, "drwav.mp3");*/
                /*if (nk_contextual_item_label(ctx, "New Color", NK_TEXT_CENTERED))*/
                /*    node_editor_add_color(nodedit, "Color", nk_rect(400, 260, 180, 220),*/
                /*             1, 2, nk_rgb(255, 255, 255));*/
                /*if (nk_contextual_item_label(ctx, grid_option[nodedit->show_grid],NK_TEXT_CENTERED))*/
                /*    nodedit->show_grid = !nodedit->show_grid;*/
                nk_contextual_end(ctx);
            }
        }
        nk_layout_space_end(ctx);

        /* window content scrolling */
        if (nk_input_is_mouse_hovering_rect(in, nk_window_get_bounds(ctx)) &&
            nk_input_is_mouse_down(in, NK_BUTTON_MIDDLE)) {
            nodedit->scrolling.x += in->mouse.delta.x;
            nodedit->scrolling.y += in->mouse.delta.y;
        }
    }
    nk_end(ctx);
    return !nk_window_is_closed(ctx, "NodeEdit");
}

