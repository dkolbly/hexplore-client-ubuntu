#version 120            // --*-c-*--

//in vec4 fragmentColor;
varying vec3 worldPosn;
varying vec2 fragmentUV;
varying float fragmentAmbient;
varying float fragmentFog;

uniform sampler2D theTextureSampler;
uniform vec4 fogColor;

void main(void) {
  //gl_FragColor = vec4(worldPosn.x, worldPosn.y, 0.2, 1);
  //gl_FragColor = fragmentColor;
  gl_FragColor = mix( fogColor, //vec4(1,1,1,1),
                      texture2D(theTextureSampler, fragmentUV) * fragmentAmbient,
                      fragmentFog );
  
  if (gl_FragColor.a < 0.5) {
    discard;
  }
}
