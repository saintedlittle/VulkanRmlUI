#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 transform;
    vec2 translation;
} pc;

void main() {
    // Apply transform and translation
    vec4 worldPos = pc.transform * vec4(inPosition + pc.translation, 0.0, 1.0);
    
    // Convert to normalized device coordinates for UI rendering
    // Assuming screen space coordinates where (0,0) is top-left
    gl_Position = worldPos;
    
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}