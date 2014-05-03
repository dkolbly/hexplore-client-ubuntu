#version 120            // --*-c-*--

varying vec3 worldPosn;
varying vec2 fragmentUV;
varying float fragmentAmbient;
varying float fragmentFog;

uniform sampler2D theTextureSampler;
uniform vec4 fogColor;
uniform vec2 waterWiggle;

void main(void) {
  gl_FragColor = mix( fogColor, //vec4(1,1,1,1),
                      texture2D(theTextureSampler, fragmentUV + waterWiggle) * fragmentAmbient,
                      fragmentFog );
}
