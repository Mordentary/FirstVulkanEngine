#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec2 a_TextureCoord;

layout(location = 0) out vec3 v_VertColor;
layout(location = 1) out vec2 v_TextureCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(a_Position, 1.0);
    v_VertColor = a_Color;
    v_TextureCoord = a_TextureCoord;
}
