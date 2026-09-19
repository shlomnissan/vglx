#version 410 core

layout (location = 0) out vec4 o_FragColor;

in vec2 v_TexCoords;

uniform sampler2D u_TextureMap;
uniform vec3 u_Color;
uniform float u_Opacity;

void main() {
    vec4 texel = texture(u_TextureMap, v_TexCoords);
    o_FragColor = vec4(texel.rgb * u_Color, texel.a * u_Opacity);
}
