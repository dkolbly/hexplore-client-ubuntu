#version 120            // --*-c-*--

attribute vec3 vertexPosition_modelspace;

varying vec3 posn;

uniform mat4 MVP;

void main()
{
  vec4 v = gl_Vertex;
  posn = vec3(MVP * v);
  gl_Position = gl_Vertex;
}
