#version 410 core

in vec3 a_Position;
in vec2 a_TexCoord;

uniform mat3 u_Projection;
uniform mat3 u_Model;
uniform vec2 u_Size;
uniform vec2 u_Anchor;
uniform vec4 u_UVRect;

out vec2 v_TexCoords;

void main() {
    vec2 local = (a_Position.xy - u_Anchor) * u_Size;
    vec3 position = u_Projection * u_Model * vec3(local, 1.0);

    v_TexCoords = u_UVRect.xy + a_TexCoord * u_UVRect.zw;
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
