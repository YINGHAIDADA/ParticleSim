#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uScene;
layout(set = 0, binding = 1) uniform sampler2D uBloom;
layout(set = 0, binding = 2) uniform sampler2D uLight;
layout(push_constant) uniform PushConstants {
    float panelU;
    float zoom;
    vec2 center;
    float toolbarV;
} pc;

vec2 sceneUV(vec2 screenUV) {
    if (screenUV.x < pc.panelU) return screenUV;
    float canvas = max(1.0 - pc.panelU, 0.0001);
    float localX = (screenUV.x - pc.panelU) / canvas;
    float zoom = max(pc.zoom, 0.05);
    return vec2(
        pc.center.x + (localX - 0.5) * canvas / zoom,
        pc.center.y + (screenUV.y - 0.5) / zoom
    );
}

void main() {
    bool inChrome = vUV.x < pc.panelU || vUV.y < pc.toolbarV;
    vec2 uv = sceneUV(vUV);
    bool inSim = uv.x >= pc.panelU && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0;
    if (!inChrome && !inSim) {
        outColor = vec4(0.1, 0.1, 0.1, 1.0);
        return;
    }

    vec4 scene = texture(uScene, inChrome ? vUV : uv);
    vec3 albedo = mix(vec3(0.1), scene.rgb, scene.a);
    if (!inChrome) {
        vec3 light = texture(uLight, vUV).rgb;
        float heat = clamp(dot(light, vec3(0.6, 0.3, 0.1)) * 4.0, 0.0, 1.6);
        albedo *= vec3(1.0) + heat * vec3(1.2, 0.38, 0.05);
        albedo += texture(uBloom, vUV).rgb * 0.40;
    }
    outColor = vec4(albedo, 1.0);
}
