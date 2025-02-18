// Basic shader
@vs vs_basic
in vec4 pos;
void main() {
    gl_Position = pos;
}
@end

@fs fs_basic
out vec4 frag_color;
void main() {
    frag_color = vec4(0.8, 0.7, 0.5, 1.0);
}
@end

@program basic vs_basic fs_basic

// Triangle shader
@vs vs_triangle
in vec4 position;
in vec4 color0;

out vec4 color;

void main() {
    gl_Position = position + 0.8;
    color = color0;
}
@end

@fs fs_triangle
in vec4 color;
out vec4 frag_color;

void main() {
    frag_color = color;
}
@end

@program triangle vs_triangle fs_triangle

// Sprite shader
@vs vs_sprite
in vec4 pos;
in vec2 texcoord0;
out vec2 uv;
void main() {
    gl_Position = pos;
    uv = texcoord0;
}
@end

@fs fs_sprite
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;

in vec2 uv;
out vec4 frag_color;

void main() {
    frag_color = texture(sampler2D(tex, smp), uv);
}
@end

@program sprite vs_sprite fs_sprite

