#include <stdlib.h>

#include "nuklear.h"

#define MAX_SAMPLES 64

typedef struct AudioVisualizer {
    float samples[MAX_SAMPLES];
    int num_samples;
    float peak_value;
    struct nk_color bar_color;
    struct nk_color background_color;
} AudioVisualizer;

void audio_visualizer_init(AudioVisualizer* viz) {
    viz->num_samples = MAX_SAMPLES;
    viz->peak_value = 1.0f;
    viz->bar_color = nk_rgb(0, 255, 128);
    viz->background_color = nk_rgb(40, 40, 40);
    
    // Initialize with dummy data
    for (int i = 0; i < MAX_SAMPLES; i++) {
        viz->samples[i] = 0.0f;
    }
}

void audio_visualizer_update(AudioVisualizer* viz, float* new_samples, int count) {
    int samples_to_copy = NK_MIN(count, MAX_SAMPLES);
    for (int i = 0; i < samples_to_copy; i++) {
        viz->samples[i] = new_samples[i];
        // Update peak value if necessary
        if (new_samples[i] > viz->peak_value) {
            viz->peak_value = new_samples[i];
        }
    }
}
void audio_visualizer_draw(struct nk_context* ctx, AudioVisualizer* viz, struct nk_rect bounds) {
    if (nk_begin(ctx, "Audio Visualizer", bounds,
        NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | 
        NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
        /*struct nk_rect bounds = nk_window_get_content_region(ctx);*/
        float bar_width = bounds.w / viz->num_samples;
        float bar_spacing = bar_width * 0.1f;
        
        // Draw background
        nk_fill_rect(&ctx->current->buffer, bounds, 0, viz->background_color);
        
        // Draw bars
        for (int i = 0; i < viz->num_samples; i++) {
            float normalized_value = viz->samples[i] / viz->peak_value;
            float bar_height = normalized_value * bounds.h;
            
            struct nk_rect bar = nk_rect(
                bounds.x + i * (bar_width),
                bounds.y + bounds.h - bar_height,
                bar_width - bar_spacing,
                bar_height
            );
            
            nk_fill_rect(&ctx->current->buffer, bar, 0, viz->bar_color);
        }
        
        // Optional: Add controls
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_button_label(ctx, "Reset Peak")) {
            viz->peak_value = 1.0f;
        }
        
        /*// Color picker for bar color*/
        /*nk_layout_row_dynamic(ctx, 25, 2);*/
        /*nk_label(ctx, "Bar Color:", NK_TEXT_LEFT);*/
        /*viz->bar_color = nk_color_picker(ctx, viz->bar_color, NK_RGB);*/
    }
    nk_end(ctx);
}

// Example usage in main application
void example_usage(struct nk_context* ctx) {
    static AudioVisualizer visualizer;
    static int initialized = 0;
    
    if (!initialized) {
        audio_visualizer_init(&visualizer);
        initialized = 1;
    }
    
    // Example: Update with dummy data
    float dummy_data[MAX_SAMPLES];
    for (int i = 0; i < MAX_SAMPLES; i++) {
        dummy_data[i] = (float)rand() / RAND_MAX;
    }
    audio_visualizer_update(&visualizer, dummy_data, MAX_SAMPLES);
    
    // Draw the visualizer
    audio_visualizer_draw(ctx, &visualizer, nk_rect(0, 0, 500, 500));
}
