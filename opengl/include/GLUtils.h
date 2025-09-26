#pragma once

#define OPENGL_VERTEX_SHADER(...) "#version 330 core\n" #__VA_ARGS__
#define OPENGL_FRAGMENT_SHADER(...) "#version 330 core\n" #__VA_ARGS__

/* Example usage:
const char* TriangleVertexShader = VERTEX_SHADER(
       layout(location = 0) in vec4 vPosition;
       void main() {
           gl_Position = vPosition;
       }
);

const char* TriangleFragmentShader = FRAGMENT_SHADER(
     precision mediump float;
     out vec4 FragColor;
     void main() {
         FragColor = vec4(0.0, 1.0, 0.0, 1.0);
     }
);
*/
