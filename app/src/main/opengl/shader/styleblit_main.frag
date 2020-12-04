#version 150

precision highp float;

uniform sampler2D stylePosGuide;
uniform sampler2D targetPosGuide;
uniform sampler2D styleAppGuide;
uniform sampler2D targetAppGuide;
uniform sampler2D styleImg;

in vec2 texCoord_v;
out vec4 fragColor;

void main() {
  //fragColor = vec4(0.0f,1.0f,0.0f,1.0f);
  fragColor = texture(styleAppGuide, texCoord_v);
}