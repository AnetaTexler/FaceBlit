#version 150

precision highp float;

#ifndef BLEND_RADIUS
  #define BLEND_RADIUS 1
#endif

uniform sampler2D NNF;
uniform sampler2D styleImg;
in vec2 texCoord_v;
out vec4 fragColor;

// Only works when source and target are of the same size.
uniform int width;
uniform int height;

vec2 unpack(vec4 rgba)
{
  return vec2(rgba.r*255.0+rgba.g*255.0*255.0,
              rgba.b*255.0+rgba.a*255.0*255.0);
}

void main() {
  // Uncomment to make the shader to just display the NNF texture. 
  // Useful if you want to display some visualization from the previous shader.
  /*fragColor = texture(NNF, texCoord_v);
  return;*/

  vec2 xy = gl_FragCoord.xy;

  vec4 sumColor = vec4(0.0,0.0,0.0,0.0);
  float sumWeight = 0.0;
  

  for(int oy=-BLEND_RADIUS;oy<=+BLEND_RADIUS;oy++)
  for(int ox=-BLEND_RADIUS;ox<=+BLEND_RADIUS;ox++)
    {
        sumColor += texture2D(styleImg,((unpack(texture2D(NNF,(xy+vec2(ox,oy))/vec2(width,height)))-vec2(ox,oy))+vec2(0.5,0.5))/vec2(width,height));
        sumWeight += 1.0;
    }
  
  fragColor = (sumWeight>0.0) ? sumColor/sumWeight : texture2D(styleImg,vec2(0.0,0.0));
}
