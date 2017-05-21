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
    //createImgTex();// Should already be done
    //createComputeShaders();
    createShaders();
    createFbos();
    createFullScreenQuad();
    createFilterTex();
    createInvLookupTex(extmode::symper);

    GLint maxview;
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &maxview);
    std::cerr << "MAXMAXMAXMAXMAXMA   " << maxview << std::endl;
}

void J2KGpu::inversedwt(/*float* imageBuffer, */int level) {
  for (int i = level - 1; i >= 0; i--) {
      inversedwtInternal(i, 0, 0, 0, 0);
  }
}

bool J2KGpu::inversedwtInternal(int level, int startx, int starty, int endx, int endy) {

  std::cerr << GL_MAX_VIEWPORT_DIMS << std::endl;
  // Activate render to fbo1 texture
  _idwtRowFbo->activate();
  // Activate row shader
  _inverseDwtRowProgram->activate();
  _inverseDwtRowProgram->setUniform("level", level);

  // Inverse lookup texture
  ghoul::opengl::TextureUnit lookupUnit;
  lookupUnit.activate();
  glBindTexture(GL_TEXTURE_RECTANGLE_NV, _lookupTexID);
  _inverseDwtRowProgram->setUniform("lut", lookupUnit);

  // Filter texture
  ghoul::opengl::TextureUnit filterUnit;
  glBindTexture(GL_TEXTURE_RECTANGLE_NV, _reconFilterTexID);
  _inverseDwtRowProgram->setUniform("filter", filterUnit);

//  glDispatchCompute(4096, 4096, 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  _inverseDwtRowProgram->deactivate();
  _idwtRowFbo->deactivate();

  // Tex1 now holds 

  //_inverseDwtCol->activate();
  // Bind textures...
  //_inverseDwtCol->deactivate();
}

void J2KGpu::createFullScreenQuad() {

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
  // Row FBO
  _idwtRowFbo = std::make_unique<ghoul::opengl::FramebufferObject>();
  _fboTex1  =   std::make_unique<ghoul::opengl::Texture>(
                    nullptr,
                    glm::size3_t(_imageSize, _imageSize, 1),
                    ghoul::opengl::Texture::Red,
                    GL_R32F,
                    GL_FLOAT,
                    ghoul::opengl::Texture::FilterMode::Nearest,
                    ghoul::opengl::Texture::WrappingMode::ClampToEdge
                );
  _idwtRowFbo->attachTexture(_fboTex1.get());

  // Col FBO
  _idwtColFbo = std::make_unique<ghoul::opengl::FramebufferObject>();
  _fboTex2 = std::make_unique<ghoul::opengl::Texture>(
                    nullptr,
                    glm::size3_t(_imageSize, _imageSize, 1),
                    ghoul::opengl::Texture::Red,
                    GL_R32F,
                    GL_FLOAT,
                    ghoul::opengl::Texture::FilterMode::Nearest,
                    ghoul::opengl::Texture::WrappingMode::ClampToEdge
                );
  _idwtColFbo->attachTexture(_fboTex2.get());
}

void J2KGpu::createShaders() {
  RenderEngine& renderEngine = OsEng.renderEngine();
  _inverseDwtRowProgram = renderEngine.buildRenderProgram("InverseDwtRowProgram",
    "${MODULE_SOLARBROWSING}/shaders/inversedwt_vs.glsl",
    "${MODULE_SOLARBROWSING}/shaders/inversedwtrow_fs.glsl"
  );

  _inverseDwtColProgram = renderEngine.buildRenderProgram("InverseDwtRowProgram",
    "${MODULE_SOLARBROWSING}/shaders/inversedwt_vs.glsl",
    "${MODULE_SOLARBROWSING}/shaders/inversedwtcol_fs.glsl"
  );

  //std::string invRowName = "InverseDwtRowProgram";
  //std::string invColName = "InverseDwtColProgram";

  //_inverseDwtRow = std::make_unique<ghoul::opengl::ProgramObject>(invRowName);
  //_inverseDwtCol = std::make_unique<ghoul::opengl::ProgramObject>(invColName);
  // auto shaderObject = std::make_unique<ghoul::opengl::ShaderObject>(
  //     ghoul::opengl::ShaderObject::ShaderType::ShaderTypeCompute,
  //     absPath("/Users/michaelnoven/workspace/OpenSpace/modules/solarbrowsing/shaders/inversedwtrow_cs.glsl"),
  //     invRowName + " Compute",
  //     ghoul::Dictionary());

  // _inverseDwtRow->attachObject(std::make_unique<ghoul::opengl::ShaderObject>(
  //     ghoul::opengl::ShaderObject::ShaderType::ShaderTypeCompute,
  //     absPath("/Users/michaelnoven/workspace/OpenSpace/modules/solarbrowsing/shaders/inversedwtrow_cs.glsl"),
  //     invRowName + " Compute",
  //     ghoul::Dictionary()
  // ));


//  _inverseDwtRow->linkProgramObject();
 // _inverseDwtRow->compileShaderObjects();

  // _inverseDwtCol->attachObject(std::make_unique<ghoul::opengl::ShaderObject>(
  //     ghoul::opengl::ShaderObject::ShaderType::ShaderTypeFragment,
  //     absPath("${MODULE_SOLARBROWSING}/shaders/inversedwtcol_cs.glsl"),
  //     invColName + " Compute",
  //     ghoul::Dictionary()
  // ));
  // _inverseDwtCol->linkProgramObject();
  // _inverseDwtCol->compileShaderObjects();
}

bool J2KGpu::createFilterTex() {
  glGenTextures(1, &_reconFilterTexID);
  //glBindTexture(GL_TEXTURE_RECTANGLE_NV, _reconFilterTexID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 18, 1, 0, GL_RED, GL_FLOAT, recon_filter);
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
  //glBindTexture(GL_TEXTURE_RECTANGLE_NV, _lookupTexID);
  glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D   (GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, texwidth, texheight, 0, GL_RGBA, GL_FLOAT, tex1);
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
