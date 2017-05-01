/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2017                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#version __CONTEXT__

#include "PowerScaling/powerScaling_vs.hglsl"

layout(location = 0) in vec3 in_position;

out vec4 vs_color;

uniform int time;
uniform int flParticleSize;
uniform int modulusDivider;

uniform vec4 fieldlineColor;
uniform vec4 fieldlineParticleColor;

uniform mat4 modelViewTransform;
// uniform mat4 modelTransform;


void main() {
    // Color every n-th vertex differently to show fieldline flow direction
    int modulus = (gl_VertexID + time) % modulusDivider;
    if ( modulus > 0 && modulus < flParticleSize) {
        vs_color = fieldlineParticleColor;
    } else {
        vs_color = fieldlineColor;
    }

    // FROM MODEL SPACE TO CAMERA SPACE
    gl_Position = modelViewTransform * vec4(in_position, 1);

    // FROM MODEL SPACE TO WORLD SPACE
    // gl_Position = modelTransform * vec4(in_position, 1);
}