#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uScene;
layout(set = 0, binding = 1) uniform sampler2D uBloom;
layout(set = 0, binding = 2) uniform sampler2D uLight;
layout(push_constant) uniform PushConstants {
    float panelU;
} pc;

void main() {
    vec4 scene = texture(uScene, vUV);
    vec3 albedo = mix(vec3(0.1), scene.rgb, scene.a);
    if (vUV.x >= pc.panelU) {
        vec3 light = texture(uLight, vUV).rgb;
        float heat = clamp(dot(light, vec3(0.6, 0.3, 0.1)) * 4.0, 0.0, 1.6);
        albedo *= vec3(1.0) + heat * vec3(1.2, 0.38, 0.05);
        albedo += texture(uBloom, vUV).rgb * 0.40;
    }
    outColor = vec4(albedo, 1.0);
}
