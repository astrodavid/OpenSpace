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

#ifndef __OPENSPACE_MODULE_SOLARBROWSING___J2KGPU___H__
#define __OPENSPACE_MODULE_SOLARBROWSING___J2KGPU___H__

#include <openspace/util/openspacemodule.h>
#include <ghoul/opengl/programobject.h>
#include <ghoul/opengl/framebufferobject.h>
#include <string>

namespace openspace {

class J2KGpu {
public:
    J2KGpu(/*float* imageBuffer, */const int imageSize/*, int level*/);
    void inversedwt(int level, ghoul::opengl::Texture* compressedTexture);

    GLuint _fboTexRowTextureId;
    GLuint _fboTexColTextureId;
    std::unique_ptr<ghoul::opengl::Texture> _fboTexRow;
    std::unique_ptr<ghoul::opengl::Texture> _fboTexCol;
private:
    enum extmode {
      per,
      symper
    };

    GLuint _lookupTexID;
    GLuint _reconFilterTexID;
    //std::unique_ptr<ghoul::opengl::ProgramObject> _inverseDwtRow;
    //std::unique_ptr<ghoul::opengl::ProgramObject> _inverseDwtCol;
    std::unique_ptr<ghoul::opengl::ProgramObject> _inverseDwtRowProgram;
    std::unique_ptr<ghoul::opengl::ProgramObject> _inverseDwtColProgram;

    std::unique_ptr<ghoul::opengl::FramebufferObject> _idwtRowFbo;
    std::unique_ptr<ghoul::opengl::FramebufferObject> _idwtColFbo;

    GLuint _fullScreenQuad;
    GLuint _vertexPositionBuffer;

    bool inversedwtInternal(int level, int startx, int starty, int endx, int endy, ghoul::opengl::Texture* compressedTexture);
    bool createFilterTex();
    void calLength(int startind, int endind, int level, int *llength, int *loffset);
    int calLevels(int startind, int endind);
    void createShaders();
    void createFbos();
    void createFullScreenQuad();
    bool createInvLookupTex(extmode mode);
    bool createIDATexture(extmode mode, float** tex1, const int& width, const int& height, int& texwidth, int& texheight);
    // boundary extension function
    int ext(int index, int datalen, extmode mode);
    unsigned int _imageSize;
    unsigned int _texwidth;
    unsigned int _texheight;
};

} // namespace openspace

#endif // __OPENSPACE_MODULE_SOLARBROWSING___J2KGPU___H__
