#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uTex;
layout(push_constant) uniform PushConstants {
    float panelU;
} pc;

void main() {
    outColor = vec4(0.0, 0.0, 0.0, 1.0);
    if (vUV.x < pc.panelU) {
        return;
    }

    vec3 texColor = texture(uTex, vUV).rgb;
    float brightness = dot(texColor, vec3(0.2126, 0.7152, 0.0722));
    if (texColor.b < 0.22 && brightness > 0.30) {
        outColor = vec4(texColor, 1.0);
    }
}
