#version 150

precision highp float;

uniform sampler2D stylePosGuide;
uniform sampler2D targetPosGuide;
uniform sampler2D styleAppGuide;
uniform sampler2D targetAppGuide;
uniform sampler2D styleImg;
uniform usampler3D LUT;

// Only works when source and target are of the same size.
uniform int width;
uniform int height;

in vec2 texCoord_v;
out vec4 fragColor;

int threshold = 50;
int lambdaPos = 10; 
int lambdaApp = 2;

/*vec3 getTargetGuide(vec2 position) {
    vec2 pos_normalized = vec2( position.x / width, position.y / height );
    return lambdaPos * texture(targetPosGuide, pos_normalized).rgb + lambdaApp * texture(targetAppGuide, pos_normalized).rgb;
}

vec3 getSourceGuide(vec2 position) {
    vec2 pos_normalized = vec2( position.x / width, position.y / height );
    return lambdaPos * texture(sourcePosGuide, pos_normalized).rgb + lambdaApp * texture(sourceAppGuide, pos_normalized).rgb;
}*/

void main() {
  //fragColor = vec4(0.0f,1.0f,0.0f,1.0f);

  vec2 position = gl_FragCoord.xy;
  vec2 pos_normalized = vec2( float(position.x) / float(width), float(position.y) / float(height) );
  vec2 target_guide = texture(targetPosGuide, pos_normalized).rg;
  vec3 out_color = vec3( texture(LUT, vec3(0.5f, target_guide.g, target_guide.r) ).rg, 0.0f );
  out_color = vec3(out_color.r / float(width), out_color.g / float(height), out_color.b);
  fragColor = vec4(out_color, 1.0f);
  // fragColor = vec4(0.0f,1.0f,0.0f,1.0f);
  // fragColor = texture(targetPosGuide, texCoord_v);


  // fragColor = texture(styleAppGuide, texCoord_v);

  /*vec2 p = gl_FragCoord.xy-vec2(0.5,0.5);
  vec2 o = ArgMinLookup(GT(p));

  for(int level=6;level>=0;level--)
  {
    vec2 q = NearestSeed(p,pow(2.0,float(level)));
    vec2 u = ArgMinLookup(GT(q));
    
    float e = sum(abs(GT(p)-GS(u+(p-q))))*255.0;
    
    if (e<threshold)
    {
      o = u+(p-q); if (inside(o,sourceSize)) { break; }
    }
  }

  fragColor = texture(styleAppGuide, texCoord_v);*/
  // gl_FragColor = pack(o);
}