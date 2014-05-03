#version 120            // --*-c-*--

uniform samplerCube cubemap;
//uniform sampler2D cubemap;
varying vec3 posn;
uniform vec3 sun;
uniform mat4 stars;

void main()
{
  float ambient = smoothstep(-0.2, 0.0, sun[2]);
  vec4 p = stars * vec4(posn,1);
  gl_FragColor = textureCube(cubemap, vec3(p)) * (1-ambient);

  float d = dot(normalize(posn),sun);

  float adj = smoothstep(0.8, 1, d);
  gl_FragColor += vec4(adj + ambient*0.8, 
                       adj + ambient*0.8, 
                       adj + ambient, 
                       1);

  if (d > 0.9985) {
    float s;
    if (d >= 0.999) {
      s = 1;
    } else {
      s = smoothstep(0.9985, 0.999, d);
    }
    /*   0 .. 0.001  solid
         0.001 0.002 smooth transition
    */
    gl_FragColor[0] = gl_FragColor[0] * (1-s) + s;
    gl_FragColor[1] = gl_FragColor[1] * (1-s) + s * 0xDD/255.0;
    gl_FragColor[2] = gl_FragColor[2] * (1-s) + s * 0x75/255.0;
  }

  
  /*
  // draw a line across the ecliptic
  if (abs(posn.y) < 0.005) {
    gl_FragColor[0] = 1;
    gl_FragColor[1] = 1;
    gl_FragColor[2] = 1;
  }
  */
}
