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

#include "fragment.glsl"

uniform sampler2DRect compressedImageryTexture;
uniform sampler2DRect filterTex;
uniform sampler2DRect lut;

uniform uint texwidth;
uniform uint texheight;
uniform float level;

in vec2 vs_st;

Fragment getFragment() {
    float bb, level_center = level + 0.5;
    vec2 st, filter_st;
    vec3 sum = vec3(0);

    // Lookup filter selector (lut.g)
    float lookup = texture(lut, vec2(texwidth - vs_st.y * 4096.0, texheight - level_center)).g;
    filter_st = vec2(0.0 + lookup * 9.0 + 0.5, 0.5);
    st = vec2(vs_st.x * 4096.0, 0.0);

    // Lookup indirect address (lut.r) & filter values (filter.x), then convolve
    for (int i = 0; i < 9; i++) {
      st.y = texture(lut, vec2(texwidth - vs_st.y * 4096.0  - i, texheight - level_center)).r;
      sum += texture(filterTex, filter_st).x * texture(compressedImageryTexture, st).xyz;
      filter_st.x += 1.0;
    }

    vec4 diffuse = vec4(sum, 1.0);
    //vec4 diffuse = texture(compressedImageryTexture, vs_st);
    //vec4 diffuse = vec4(vs_st.x, vs_st.y/ 4096.0, 1.0, 1.0);

    Fragment frag;
    frag.color = diffuse;
    frag.depth = 1.0;
    return frag;
}
