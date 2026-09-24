#version 410 core

in vec3 a_Position;
in vec2 a_TexCoord;

uniform mat3 u_Projection;
uniform mat3 u_Model;
uniform mat3 u_TextureTransform;

out vec2 v_TexCoords;

void main() {
    vec3 position = u_Projection * u_Model * vec3(a_Position.xy, 1.0);

    v_TexCoords = (u_TextureTransform * vec3(a_TexCoord, 1.0)).xy;
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
