/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2016                                                               *
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

uniform vec4 campos;
uniform vec4 objpos;
//uniform vec3 camdir; // add this for specular
  

uniform float time;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D heightTex;
uniform sampler2D normalTex;

uniform bool _enableNormalMapping;

in vec2 vs_st;
in vec4 vs_normal;
in vec4 vs_position;

in vec4 ProjTexCoord;

uniform vec3 boresight;
uniform vec3 sun_pos;

#include "PowerScaling/powerScaling_fs.hglsl"
#include "fragment.glsl"

Fragment getFragment() {
    vec4 position = vs_position;
    float depth = pscDepth(position);
    vec4 diffuse = texture(texture1, vs_st);
    // vec4 diffuse = texture(heightTex, vs_st);

    // directional lighting
    vec3 origin = vec3(0.0);
    vec4 spec = vec4(0.0);
    
    // vec3 n = normalize(texture(normalTex, vs_st).xyz);

    vec3 n = normalize(vs_normal.xyz);
    // n = vec3(0);
    // vec3 n = vec3(0);

    if (_enableNormalMapping) {
        n = n + normalize(texture(normalTex, vs_st).xyz);
    }
    //vec3 e = normalize(camdir);
    vec3 l_pos = sun_pos; // sun.
    vec3 l_dir = normalize(l_pos-objpos.xyz);
    float terminatorBright = 0.4;
    float intensity = min(max(5*dot(n,l_dir), terminatorBright), 1);
    
    float shine = 0.0001;

    vec4 specular = vec4(0.1);
    vec4 ambient = vec4(0.f,0.f,0.f,1);
    /* Specular
    if(intensity > 0.f){
        // halfway vector
        vec3 h = normalize(l_dir + e);
        // specular factor
        float intSpec = max(dot(h,n),0.0);
        spec = specular * pow(intSpec, shine);
    }
    */
    //diffuse = max(intensity * diffuse, ambient);
    
    // PROJECTIVE TEXTURE
    vec4 projTexColor = textureProj(texture2, ProjTexCoord);
    vec4 shaded = max(intensity * diffuse, ambient);
    if (ProjTexCoord[0] > 0.0 ||
        ProjTexCoord[1] > 0.0 ||
        ProjTexCoord[0] < ProjTexCoord[2] ||
        ProjTexCoord[1] < ProjTexCoord[2]){
        diffuse = shaded;
    } else if (dot(n,boresight) < 0 &&
              (projTexColor.w != 0)) {// frontfacing
        diffuse = projTexColor;//*0.5f + 0.5f*shaded;
    } else {
        diffuse = shaded;
    }

    Fragment frag;
    frag.color = diffuse;
// frag.color = vec4(normalize(vs_position.xyz), 1.0);
    
    // frag.color =  vec4(vec3(texture(heightTex, vs_st).r), 1.0);
    // frag.color = texture(heightTex, vs_st);
// frag.color = vec4(n, 1.0);
    // frag.color = test;
    frag.depth = depth;
    //frag.color = vec4(vs_st, 0.0, 1.0);
    return frag;
}