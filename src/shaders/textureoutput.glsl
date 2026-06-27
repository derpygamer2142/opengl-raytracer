#version 330 core


in vec4 gl_FragCoord;
in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D inputTexture;

void main() {
    fragColor = texture(inputTexture, texCoord);
}