/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014                                                                    *
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

uniform mat4 modelViewProjection;
uniform mat4 modelTransform;
uniform int time;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;

out vec4 vs_color;
out vec4 vs_position;

#include "PowerScaling/powerScaling_vs.hglsl"

void main() {
    // Color every n-th vertex differently to show fieldline flow direction
    if ( (gl_VertexID + time) % 20 == 0) {
        vs_color = vec4(in_color.rgb * 1.0, 0.9);
    } else {
        vs_color = vec4(in_color.rgb * 0.7, 0.6);
    }

    vec4 tmp = vec4(in_position, 0);

    vec4 position_meters = pscTransform(tmp, modelTransform);
    vs_position = tmp;

    // project the position to view space
    position_meters =  modelViewProjection * position_meters;
    gl_Position = z_normalization(position_meters);
}