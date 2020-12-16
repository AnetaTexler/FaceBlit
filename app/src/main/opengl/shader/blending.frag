#version 150

precision highp float;

#ifndef BLEND_RADIUS
  #define BLEND_RADIUS 100
#endif

uniform sampler2D A;
uniform sampler2D B;
uniform sampler2D mask;
uniform sampler2D facial_mask;
uniform sampler2D background;
in vec2 texCoord_v;
out vec4 fragColor;

uniform int width;
uniform int height;

void main() {
  //fragColor = vec4(0.0f,1.0f,0.0f,1.0f);
  //fragColor = texture(A, texCoord_v);

  float facial_mask = texture(facial_mask, texCoord_v).r;
  float alpha = max(texture(mask, texCoord_v).r - facial_mask, 0.0f);
  float remained = 1.0f - (facial_mask + alpha);
  /*fragColor = vec4(remained,remained,remained,1.0f);
  return;*/

  fragColor = alpha * texture(A, texCoord_v) + facial_mask * texture(B, texCoord_v) + remained * texture(background, texCoord_v);
}