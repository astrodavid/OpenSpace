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

#include <modules/iswa/rendering/texturecygnet.h>

#include <ghoul/io/texture/texturereader.h>

namespace {
    const std::string _loggerCat = "TextureCygnet";
}

namespace openspace{

TextureCygnet::TextureCygnet(const ghoul::Dictionary& dictionary)
    :IswaCygnet(dictionary)
{ 
    registerProperties();
}

TextureCygnet::~TextureCygnet(){}

bool TextureCygnet::updateTexture() {

    std::unique_ptr<ghoul::opengl::Texture> texture = ghoul::io::TextureReader::ref().loadTexture(
                                                        (void*) _imageFile.buffer,
                                                        _imageFile.size, 
                                                        _imageFile.format);

    if (texture) {
        LDEBUG("Loaded texture from image iswa cygnet with id: '" << _data->id << "'");
        texture->uploadTexture();
        // Textures of planets looks much smoother with AnisotropicMipMap rather than linear
        texture->setFilter(ghoul::opengl::Texture::FilterMode::Linear);
        _textures[0]  = std::move(texture);
    }

    return false;
}

bool TextureCygnet::downloadTextureResource(){

    if(_futureObject.valid())
        return false;

    if(_textures.empty())
        _textures.push_back(nullptr);

    std::future<DownloadManager::MemoryFile> future = IswaManager::ref().fetchImageCygnet(_data->id);

    if(future.valid()){
        _futureObject = std::move(future);
        return true;
    }

    return false;
}

bool TextureCygnet::updateTextureResource(){

    // if The future is done then get the new imageFile
    DownloadManager::MemoryFile imageFile;
    if(_futureObject.valid() && DownloadManager::futureReady(_futureObject)){
        imageFile = _futureObject.get();

        if(imageFile.corrupted){
            if(imageFile.buffer)
                delete[] imageFile.buffer;
            return false;
        } else {
            _imageFile = imageFile;
        }
    } else {
        return false;
    }

    return true;
}

bool TextureCygnet::readyToRender() const {
    return (isReady() && ((!_textures.empty()) && (_textures[0] != nullptr)));
}


} //namespace openspace