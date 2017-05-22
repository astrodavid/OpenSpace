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

#include <modules/solarbrowsing/util/j2kgpu.h>
#include <math.h>
#include <ghoul/filesystem/filesystem.h>
#include <ghoul/opengl/textureunit.h>
#include <modules/solarbrowsing/rendering/renderablespacecraftcameraplane.h>
#include <iostream>
#include <openspace/engine/openspaceengine.h>

// 1. Decode job (buffer) is only to read in file. => Buffer will be constantly full
// 2. Send texture as usual to GPU. But with RAW data
// 3. Switch on compute program, Dispatch Compute Shader Row
// 4. Swithc on other compute program, Dispatch Compute Shader Col
// 4. Read from texture as usual in fragment shader

namespace {
  const float recon_filter[18] = {
    0,  0.033728f, -0.057544f, -0.533728f, 1.115087f,-0.533728f, -0.057544f,  0.033728f, 0, 
    0.053498f, -0.091272f, -0.156446f,  0.591272f, 1.205898f, 0.591272f, -0.156446f, -0.091272f, 0.053498f
  };
}

namespace openspace {

J2KGpu::J2KGpu(const int imageSize) {
    _imageSize = imageSize;
    createShaders();
    createFbos();
    createFullScreenQuad();
    createFilterTex();
    createInvLookupTex(extmode::symper);

    resBuffer = new float[4096 * 4096];
}

void J2KGpu::inversedwt(/*float* imageBuffer, */int level, ghoul::opengl::Texture* compressedTexture) {
  for (int i = level - 1; i >= 0; i--) {
    inversedwtInternal(i, 0, 0, 0, 0, compressedTexture);
  }
}

bool J2KGpu::inversedwtInternal(int level, int startx, int starty, int endx, int endy, ghoul::opengl::Texture* compressedTexture) {
  if (_inverseDwtRowProgram->isDirty()) {
      _inverseDwtRowProgram->rebuildFromFile();
  }

  if (_inverseDwtColProgram->isDirty()) {
      _inverseDwtColProgram->rebuildFromFile();
  }

  //============== ROW ================//

  // Activate Fbo Row
  _idwtRowFbo->activate();
  GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Framebuffer error " << status;
  }
  // Activate Row shader
  _inverseDwtRowProgram->activate();

  // Set level uniform
  _inverseDwtRowProgram->setUniform("level", static_cast<float>(level));
  _inverseDwtRowProgram->setUniform("levelTest", static_cast<float>(3.f));

  // Bind compressed texture
  ghoul::opengl::TextureUnit imageUnit;
  imageUnit.activate();
  compressedTexture->bind();
  _inverseDwtRowProgram->setUniform("compressedImageryTexture", imageUnit);

  // Bind inverse lookup texture
  ghoul::opengl::TextureUnit lookupUnit;
  lookupUnit.activate();
  glBindTexture(GL_TEXTURE_RECTANGLE, _lookupTexID);
  _inverseDwtRowProgram->setUniform("lut", lookupUnit);

  // // Bind filter texture
  ghoul::opengl::TextureUnit filterUnit;
  filterUnit.activate();
  glBindTexture(GL_TEXTURE_RECTANGLE, _reconFilterTexID);
  _inverseDwtRowProgram->setUniform("filterTex", filterUnit);

  // Disable Scissor test if enabled..
  const bool isScissorTestEnabled = glIsEnabled(GL_SCISSOR_TEST);
  if (isScissorTestEnabled) {
    glDisable(GL_SCISSOR_TEST);
  }
  // Get current viewport size
  GLint viewPortSize[4];
  glGetIntegerv(GL_VIEWPORT, viewPortSize);
  glViewport(0, 0, 4096, 4096);

  glBindVertexArray(_fullScreenQuad);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  _inverseDwtRowProgram->deactivate();
  _idwtRowFbo->deactivate();

  //============== COL ================//
  _idwtColFbo->activate();
  _inverseDwtColProgram->activate();

  // Set uniforms
  _inverseDwtColProgram->setUniform("level", static_cast<float>(level));
  _inverseDwtColProgram->setUniform("texwidth", _texwidth);
  _inverseDwtColProgram->setUniform("texheight", _texheight);

  _inverseDwtColProgram->setUniform("lut", lookupUnit);
  _inverseDwtColProgram->setUniform("filterTex", filterUnit);

  // Bind texture from previous fbo
  ghoul::opengl::TextureUnit imageUnit2;
  imageUnit2.activate();
  glBindTexture(GL_TEXTURE_RECTANGLE, _fboTexRowTextureId);
  _inverseDwtColProgram->setUniform("compressedImageryTexture", imageUnit2);

  // Reset
  glBindVertexArray(_fullScreenQuad);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // Read back pixels into buffer
  glReadPixels(0, 0, 4096, 4096, GL_RED, GL_FLOAT, resBuffer);

  _inverseDwtColProgram->deactivate();
  _idwtColFbo->deactivate();
  // // Set back viewport size
  glViewport(viewPortSize[0], viewPortSize[1], viewPortSize[2], viewPortSize[3]);
  // Activate scissor test
  if (isScissorTestEnabled) {
    glEnable(GL_SCISSOR_TEST);
  }
}

void J2KGpu::createFullScreenQuad() {
  glGenVertexArrays(1, &_fullScreenQuad); // generate array
  glGenBuffers(1, &_vertexPositionBuffer);

  const GLfloat size = 1.f;
  const GLfloat vertex_data[] = {
      //      x      y     z     w     s     t
      -size, -size, 0.f, 0.f, 0.f, 0.f,
      size, size, 0.f, 0.f, 1.f, 1.f,
      -size, size, 0.f, 0.f, 0.f, 1.f,
      -size, -size, 0.f, 0.f, 0.f, 0.f,
      size, -size, 0.f, 0.f, 1.f, 0.f,
      size, size, 0.f, 0.f, 1.f, 1.f,
  };

  glBindVertexArray(_fullScreenQuad); // bind array
  glBindBuffer(GL_ARRAY_BUFFER, _vertexPositionBuffer); // bind buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6,
                        reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6,
                        reinterpret_cast<void*>(sizeof(GLfloat) * 4));
}

void J2KGpu::createFbos() {
  //============== ROW ================//
  // Generate texture
  _fboTexRowTextureId = 0;
  glGenTextures(1, &_fboTexRowTextureId);
  // Bind and generate texture parameters
  glBindTexture(GL_TEXTURE_RECTANGLE, _fboTexRowTextureId);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  // Allocate some memory
  unsigned int arraySize = _imageSize * _imageSize * sizeof(float) * 4;
  GLubyte* _pixels = new GLubyte[arraySize];
  std::memset(_pixels, 0, arraySize);
  glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, _imageSize, _imageSize, 0, GL_RGBA, GL_FLOAT, _pixels);

  _idwtRowFbo = std::make_unique<ghoul::opengl::FramebufferObject>();
  _idwtRowFbo->activate();
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, _fboTexRowTextureId, 0);
  _idwtRowFbo->deactivate();

  //============== COL ================//
  _fboTexColTextureId = 0;
  glGenTextures(1, &_fboTexColTextureId);

  // Bind texture for col fbo
  glBindTexture(GL_TEXTURE_RECTANGLE, _fboTexColTextureId);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  // Allocate some memory
  GLubyte* _pixels1 = new GLubyte[arraySize];
  std::memset(_pixels1, 0, arraySize);

  glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, _imageSize, _imageSize, 0, GL_RGBA, GL_FLOAT, _pixels1);
  _idwtColFbo = std::make_unique<ghoul::opengl::FramebufferObject>();
  _idwtColFbo->activate();
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, _fboTexColTextureId, 0);
  _idwtColFbo->deactivate();
}

void J2KGpu::createShaders() {
  RenderEngine& renderEngine = OsEng.renderEngine();
  _inverseDwtRowProgram = renderEngine.buildRenderProgram("InverseDwtRowProgram",
    "${MODULE_SOLARBROWSING}/shaders/inversedwt_vs.glsl",
    "${MODULE_SOLARBROWSING}/shaders/inversedwtrow_fs.glsl"
  );

  _inverseDwtColProgram = renderEngine.buildRenderProgram("InverseDwtColProgram",
    "${MODULE_SOLARBROWSING}/shaders/inversedwt_vs.glsl",
    "${MODULE_SOLARBROWSING}/shaders/inversedwtcol_fs.glsl"
  );
}

bool J2KGpu::createFilterTex() {
  glGenTextures(1, &_reconFilterTexID);
  glBindTexture(GL_TEXTURE_RECTANGLE, _reconFilterTexID);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R32F, 18, 1, 0, GL_RED, GL_FLOAT, recon_filter);
}

bool J2KGpu::createInvLookupTex(extmode mode) {
  float* tex1 = nullptr;
  int texwidth;
  int texheight;

  createIDATexture(mode, &tex1, _imageSize, _imageSize, texwidth, texheight);
  _texwidth = texwidth;
  _texheight = texheight;

  glGenTextures(1, &_lookupTexID);
  // Bind texture for IDWT
  glBindTexture(GL_TEXTURE_RECTANGLE, _lookupTexID);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D   (GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, texwidth, texheight, 0, GL_RGBA, GL_FLOAT, tex1);
  // Given OpenGL the texture memory, safe to delete
  delete[] tex1;
}

int J2KGpu::ext(int index, int datalen, extmode mode) {
  int id = index;
  int eo = datalen % 2;

  // symmetric padding
  switch (mode) {
    case symper:

      if (id < 0)
        id = -id;

      if (id >= datalen)
        id = datalen - 1 - (id - datalen + 1);

      return id;
      break;
    //periodic padding
    case per:
      id = (id + datalen + eo) % (datalen + eo);
      if (eo == 0)
        return id;
      else {
        if (id != datalen)
          return id;
        else
          return datalen - 1;
      }
      break;

    default:
      id = (id + datalen) % datalen;
      return id;
      break;
  }
}

bool J2KGpu::createIDATexture(extmode mode, float** itex1, const int& width, const int& height, int& texwidth, int& texheight) {
  int startx = 0;
  int starty = 0;
  int endx = 0;
  int endy = 0;

  if (width < 1 || height < 1)
    return false;

  int i, j, k;
  int levellength, halflength, loffset;
  int L = width > height ? width : height;
  int xlevels = calLevels(startx, endx) + 1;
  int ylevels = calLevels(starty, endy) + 1;
  int maxlevels = xlevels > ylevels ? xlevels : ylevels;    //maximum number of decomposition levels for this image

  texwidth = L + 1 + 8 + 8;     // the (8 + 8) is due to boundary extension
  texheight = maxlevels;

  // lookup table holding indices for indirect addressing
  float   (*tex1)[4] = new float[texwidth * texheight][4];

  // create the indirect addressing texture
  for (i = 0; i < xlevels; i++) {
    calLength(startx, endx, i, &levellength, &loffset);
    calLength(startx, endx, i + 1, &halflength, &loffset);

    for (j = 0; j < levellength; j++) {
      int   eo = j % 2;
      float neighbours[9];

      // put filter selector to tex3
      if (loffset == 1)
        tex1[i * texwidth + j][1] = 1 - eo;
      else
        tex1[i * texwidth + j][1] = eo;
      tex1[i * texwidth + j][2] = 0;


      // Calculate neighbor indirect address (offset = -4), and put it to tex1
      if (loffset == 1)
        neighbours[0] = 0.5 + halflength * (1.0 - eo) + (ext(j - 4, levellength, mode) - 1.0 * eo) / 2.0;
      else
        neighbours[0] = 0.5 + halflength * eo + (ext(j - 4, levellength, mode) - 1.0 * eo) / 2.0;
      tex1[i * texwidth + j][0] = neighbours[0];

      // Compute 9 neighbors' addresses only for the last element (boundary extension)
      if (j == levellength - 1) {
        //  these 5 neighbours share same parity (even/oddness) 
        for (k = -4; k <= 4; k += 2) {
          if (loffset == 1)
            neighbours[k + 4] = 0.5 + halflength * (1.0 - eo) + (ext(j + k, levellength, mode) - 1.0 * eo) / 2.0;
          else
            neighbours[k + 4] = 0.5 + halflength * eo + (ext(j + k, levellength, mode) - 1.0 * eo) / 2.0;
        }

        eo = 1 - eo;

        //  these 4 neighbours share same parity (even/oddness) 
        for (k = -3; k <= 3; k += 2) {
          if (loffset == 1)
            neighbours[k + 4] = 0.5 + halflength * (1.0 - eo) + (ext(j + k, levellength, mode) - 1.0 * eo) / 2.0;
          else
            neighbours[k + 4] = 0.5 + halflength * eo + (ext(j + k, levellength, mode) - 1.0 * eo) / 2.0;
        }

        for (k = 0; k < 9; k++) // compute 9 neighbor's indirect address
          tex1[i * texwidth + j + k][0] = neighbours[k];
      }
    }
  }


  for (i = 0; i < ylevels; i++) {
    calLength(starty, endy, i, &levellength, &loffset);
    calLength(starty, endy, i + 1, &halflength, &loffset);

    for (j = 0; j < levellength; j++) {
      int   eo = j % 2;
      float neighbours[9];

      // put filter selector to tex1
      if (loffset == 1)
        tex1[(maxlevels - i - 1) * texwidth + (texwidth - 1) - j][1] = 1 - eo;
      else
        tex1[(maxlevels - i - 1) * texwidth + (texwidth - 1) - j][1] = eo;
      tex1[(maxlevels - i - 1) * texwidth + (texwidth - 1) - j][2] = 0;


      // Calculate neighbor indirect address (offset = -4), and put it to tex1            
      if (loffset == 1)
        neighbours[0] = 0.5 + halflength * (1.0 - eo) + (ext(j - 4, levellength, mode) - 1.0 * eo) / 2.0;
      else
        neighbours[0] = 0.5 + halflength * eo + (ext(j - 4, levellength, mode) - 1.0 * eo) / 2.0;
      tex1[(maxlevels - i - 1) * texwidth + (texwidth - 1) - j][0] = neighbours[0];


      // Compute 9 neighbors' addresses only for the last element (boundary extension)
      if (j == levellength - 1)
      {
        //  these 5 neighbour share same parity (even/oddness) 
        for (k = -4; k <= 4; k += 2)
        {
          if (loffset == 1)
            neighbours[k + 4] = 0.5 + halflength * (1.0 - eo) + (ext(j + k, levellength, mode) - 1.0 * eo) / 2.0;
          else
            neighbours[k + 4] = 0.5 + halflength * eo + (ext(j + k, levellength, mode) - 1.0 * eo) / 2.0;
        }

        eo = 1 - eo;

        //  these 4 neighbour share same parity (even/oddness) 
        for (k = -3; k <= 3; k += 2)
        {
          if (loffset == 1)
            neighbours[k + 4] = 0.5 + halflength * (1.0 - eo) + (ext(j + k, levellength, mode) - 1.0 * eo) / 2.0;
          else
            neighbours[k + 4] = 0.5 + halflength * eo + (ext(j + k, levellength, mode) - 1.0 * eo) / 2.0;
        }

        for (k = 0; k < 9; k++)
          tex1[(maxlevels - i - 1) * texwidth + (texwidth - 1) - j - k][0] = neighbours[k];
      }
    }
  }

  *itex1 = (float *) tex1;
  return true;
}

int J2KGpu::calLevels(int startind, int endind) {
  int level, lstart, lend;
  lstart = startind;
  lend   = endind;
  level  = 1;

  while (lend - lstart > 1) {
    lstart = ceil(lstart / 2.0f);
    lend = ceil(lend / 2.0f);
    level++;
  }
  return level;
}

void J2KGpu::calLength(int startind, int endind, int level, int *llength, int *loffset) {

  int hstart, lstart, hend, lend, i;

  lstart = startind;
  lend   = endind;
  hstart = startind;
  hend   = endind;

  for (i = 0; i < level; i++) {
    hstart = floor(lstart / 2.0f);
    hend   = floor(lend   / 2.0f);
    lstart = ceil (lstart / 2.0f);
    lend   = ceil (lend   / 2.0f);
  }

  // low-passed signal length
  *llength = lend - lstart;

  // controls the downsampling, i.e, the even/oddness of the filtered samples to be taken for lowpassed-part
  if (hstart < lstart)
    *loffset = 1;
  else
    *loffset = 0;
}
}
