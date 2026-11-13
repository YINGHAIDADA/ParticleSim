#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uTex;
layout(push_constant) uniform PushConstants {
    vec2 texel;
} pc;

const float kWeight[11] = float[11](
    0.0093, 0.028002, 0.065984, 0.121703, 0.175713, 0.198596,
    0.175713, 0.121703, 0.065984, 0.028002, 0.0093
);

void main() {
    vec3 color = vec3(0.0);
    for (int i = -5; i <= 5; ++i) {
        color += texture(uTex, vUV + pc.texel * float(i)).rgb * kWeight[i + 5];
    }
    outColor = vec4(color, 1.0);
}
