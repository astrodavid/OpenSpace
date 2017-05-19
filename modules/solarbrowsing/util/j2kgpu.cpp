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

// 1. Decode job (buffer) is only to read in file. => Buffer will be constantly full
// 2. Send texture as usual to GPU. But with RAW data
// 3. Switch on compute program, Dispatch Compute Shader Row
// 4. Swithc on other compute program, Dispatch Compute Shader Col
// 4. Read from texture as usual in fragment shader

namespace openspace {

J2KGpu::J2KGpu(float* imageBuffer, const int& imageSize, int level) {
    _imageSize = imageSize;
    createInvLookupTex(extmode::symper);
}

bool J2KGpu::createInvLookupTex(extmode mode) {
  float* tex1 = nullptr;
  int texwidth;
  int texheight;

  createIDATexture(mode, &tex1, _imageSize, _imageSize, texwidth, texheight);
  _texwidth = texwidth;
  _texheight = texheight;
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
  for (i = 0; i < xlevels; i++)
  {
    calLength(startx, endx, i, &levellength, &loffset);
    calLength(startx, endx, i + 1, &halflength, &loffset);

    for (j = 0; j < levellength; j++)
    {
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
      if (j == levellength - 1)
      {
        //  these 5 neighbours share same parity (even/oddness) 
        for (k = -4; k <= 4; k += 2)
        {
          if (loffset == 1)
            neighbours[k + 4] = 0.5 + halflength * (1.0 - eo) + (ext(j + k, levellength, mode) - 1.0 * eo) / 2.0;
          else
            neighbours[k + 4] = 0.5 + halflength * eo + (ext(j + k, levellength, mode) - 1.0 * eo) / 2.0;
        }

        eo = 1 - eo;

        //  these 4 neighbours share same parity (even/oddness) 
        for (k = -3; k <= 3; k += 2)
        {
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


  for (i = 0; i < ylevels; i++)
  {
    calLength(starty, endy, i, &levellength, &loffset);
    calLength(starty, endy, i + 1, &halflength, &loffset);

    for (j = 0; j < levellength; j++)
    {
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
