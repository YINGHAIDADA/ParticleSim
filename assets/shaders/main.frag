#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uScene;
layout(set = 0, binding = 1) uniform sampler2D uBloom;
layout(push_constant) uniform PushConstants {
    float panelU;
} pc;

void main() {
    vec4 scene = texture(uScene, vUV);
    vec3 color = mix(vec3(0.1), scene.rgb, scene.a);
    if (vUV.x >= pc.panelU) {
        color += texture(uBloom, vUV).rgb * 0.65;
    }
    outColor = vec4(color, 1.0);
}
