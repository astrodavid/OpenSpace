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

#include <openspace/rendering/renderableflare.h>
#include <openspace/engine/openspaceengine.h>
#include <openspace/rendering/flare/tsp.h>
#include <openspace/rendering/flare/brickmanager.h>

// ghoul includes
#include <ghoul/opengl/framebufferobject.h>
#include <ghoul/opengl/programobject.h>
#include <ghoul/opengl/texture.h>
#include <ghoul/opengl/textureunit.h>
#include <ghoul/opengl/texturereader.h>
#include <ghoul/filesystem/filesystem.h>
#include <ghoul/logging/logmanager.h>

// other
#include <sgct.h>

using ghoul::opengl::FramebufferObject;
using ghoul::opengl::Texture;

namespace {
	const std::string _loggerCat = "RenderableFlare";

	const std::string keyDataSource = "Source";
	const std::string keyTransferFunction = "TransferFunction";
	const std::string keyWorksizeX = "local_worksize_x";
	const std::string keyWorksizeY = "local_worksize_y";
	const std::string keyTraversalStepsize = "tsp_traveral_stepsize";
	const std::string keyRaycasterStepsize = "raycaster_stepsize";
	const std::string keyTSPTraversal = "TSPTraversal";
	const std::string keyRaycasterTSP = "raycasterTSP";
}

namespace openspace {

RenderableFlare::RenderableFlare(const ghoul::Dictionary& dictionary)
	: RenderableVolume(dictionary)
	, _tsp(nullptr)
	, _brickManager(nullptr)
	, _boxArray(0)
	, dispatch_buffer(0)
	, _tspTraversal(nullptr)
	, _raycasterTsp(nullptr)
	, _cubeProgram(nullptr)
	, _textureToAbuffer(nullptr)
	, _fbo(nullptr)
	, _backTexture(nullptr)
	, _frontTexture(nullptr)
	, _outputTexture(nullptr)
	, _transferFunction(nullptr)
{
	std::string s;
	dictionary.getValue(keyDataSource, s);
	s = absPath(s);
	if (!FileSys.fileExists(s, true))
		return;

	_tsp = new TSP(s);
	_brickManager = new BrickManager(s);

	std::string transferfunctionPath;
	if (dictionary.getValue(keyTransferFunction, transferfunctionPath)) {
		transferfunctionPath = findPath(transferfunctionPath);
		if (transferfunctionPath != "")
			_transferFunction = loadTransferFunction(transferfunctionPath);
		if (!_transferFunction)
			LERROR("Could not load transferfunction");
	}

	dictionary.getValue(keyTSPTraversal, _traversalPath);
	dictionary.getValue(keyRaycasterTSP, _raycasterPath);
	_traversalPath = findPath(_traversalPath);
	_raycasterPath = findPath(_raycasterPath);
	
	setBoundingSphere(PowerScaledScalar(2.0, 0.0));
}

RenderableFlare::~RenderableFlare() {
	if (_tsp)
		delete _tsp;
	if (_brickManager)
		delete _brickManager;
	if (dispatch_buffer)
		glDeleteBuffers(1, &dispatch_buffer);
	if (_boxArray)
		glDeleteVertexArrays(1, &_boxArray);
	if (_tspTraversal)
		delete _tspTraversal;
	if (_raycasterTsp)
		delete _raycasterTsp;
	if (_cubeProgram)
		delete _cubeProgram;
	if (_textureToAbuffer)
		delete _textureToAbuffer;
	if (_fbo)
		delete _fbo;
	if (_backTexture)
		delete _backTexture;
	if (_frontTexture)
		delete _frontTexture;
	if (_transferFunction)
		delete _transferFunction;
}

bool RenderableFlare::initialize() {
	bool success = true;

	if (_tsp) {
		success |= _tsp->load();
	}
	if (_brickManager) {
		success |= _brickManager->readHeader();
		success |= _brickManager->initialize();
	}
	
	if (success) {

		OsEng.configurationManager().getValue("pscColorToTexture", _cubeProgram);
		OsEng.configurationManager().getValue("pscTextureToABuffer", _textureToAbuffer);


		ghoul::opengl::ShaderObject* tspTraversalObject = new ghoul::opengl::ShaderObject(
			ghoul::opengl::ShaderObject::ShaderType::ShaderTypeCompute,
			_traversalPath,
			std::string("_tspTraversal CS") );

		_tspTraversal = new ghoul::opengl::ProgramObject("_tspTraversal");
		_tspTraversal->attachObject(tspTraversalObject);
		if (!_tspTraversal->compileShaderObjects())
			LERROR("Could not compile shader objects");
		if (!_tspTraversal->linkProgramObject())
			LERROR("Could not link shader objects");

		_tspTraversal->activate();

		static const struct
		{
			GLuint num_groups_x;
			GLuint num_groups_y;
			GLuint num_groups_z;
		} dispatch_params = { 1280 / 16, 720 / 16, 1 };
		glGenBuffers(1, &dispatch_buffer);
		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, dispatch_buffer);
		glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(dispatch_params), &dispatch_params, GL_STATIC_DRAW);
		_tspTraversal->deactivate();

		/*
		ghoul::opengl::ShaderObject* raycasterTSPObject = new ghoul::opengl::ShaderObject(
			ghoul::opengl::ShaderObject::ShaderType::ShaderTypeCompute,
			_raycasterPath,
			std::string("_tspTraversal CS"));

		_raycasterTsp = new ghoul::opengl::ProgramObject("_tspTraversal");
		_raycasterTsp->attachObject(raycasterTSPObject);
		if (!_raycasterTsp->compileShaderObjects())
			LERROR("Could not compile shader objects");
		if (!_raycasterTsp->linkProgramObject())
			LERROR("Could not link shader objects");

			_raycasterTsp->activate();

			static const struct
			{
			GLuint num_groups_x;
			GLuint num_groups_y;
			GLuint num_groups_z;
			} dispatch_params = { 1280 / 16, 720 / 16, 1 };
			glGenBuffers(1, &dispatch_buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, dispatch_buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(dispatch_params), &dispatch_params, GL_STATIC_DRAW);
			_raycasterTsp->deactivate();
		*/

		// ============================
		//      GEOMETRY (box)
		// ============================
		const GLfloat size = 0.5f;
		const GLfloat _w = 0.0f;
		const GLfloat vertex_data[] = {
			//  x,     y,     z,     s,
			-size, -size,  size,  _w,  0.0,  0.0,  1.0,  1.0,
			 size, -size,  size,  _w,  1.0,  0.0,  1.0,  1.0,
			 size,  size,  size,  _w,  1.0,  1.0,  1.0,  1.0,
			-size,  size,  size,  _w,  0.0,  1.0,  1.0,  1.0,
			-size, -size,  size,  _w,  0.0,  0.0,  1.0,  1.0,
			 size,  size,  size,  _w,  1.0,  1.0,  1.0,  1.0,

			-size, -size, -size,  _w,  0.0,  0.0,  0.0,  1.0,
			-size,  size, -size,  _w,  0.0,  1.0,  0.0,  1.0,
			 size,  size, -size,  _w,  1.0,  1.0,  0.0,  1.0,
			-size, -size, -size,  _w,  0.0,  0.0,  0.0,  1.0,
			 size,  size, -size,  _w,  1.0,  1.0,  0.0,  1.0,
			 size, -size, -size,  _w,  1.0,  0.0,  0.0,  1.0,

			 size, -size, -size,  _w,  1.0,  0.0,  0.0,  1.0,
			 size,  size,  size,  _w,  1.0,  1.0,  1.0,  1.0,
			 size, -size,  size,  _w,  1.0,  0.0,  1.0,  1.0,
			 size, -size, -size,  _w,  1.0,  0.0,  0.0,  1.0,
			 size,  size, -size,  _w,  1.0,  1.0,  0.0,  1.0,
			 size,  size,  size,  _w,  1.0,  1.0,  1.0,  1.0,

			-size, -size, -size,  _w,  0.0,  0.0,  0.0,  1.0,
			-size,  size,  size,  _w,  0.0,  1.0,  1.0,  1.0,
			-size,  size, -size,  _w,  0.0,  1.0,  0.0,  1.0,
			-size, -size, -size,  _w,  0.0,  0.0,  0.0,  1.0,
			-size, -size,  size,  _w,  0.0,  0.0,  1.0,  1.0,
			-size,  size,  size,  _w,  0.0,  1.0,  1.0,  1.0,

			-size,  size, -size,  _w,  0.0,  1.0,  0.0,  1.0,
			-size,  size,  size,  _w,  0.0,  1.0,  1.0,  1.0,
			 size,  size,  size,  _w,  1.0,  1.0,  1.0,  1.0,
			-size,  size, -size,  _w,  0.0,  1.0,  0.0,  1.0,
			 size,  size,  size,  _w,  1.0,  1.0,  1.0,  1.0,
			 size,  size, -size,  _w,  1.0,  1.0,  0.0,  1.0,

			-size, -size, -size,  _w,  0.0,  0.0,  0.0,  1.0,
			 size, -size,  size,  _w,  1.0,  0.0,  1.0,  1.0,
			-size, -size,  size,  _w,  0.0,  0.0,  1.0,  1.0,
			-size, -size, -size,  _w,  0.0,  0.0,  0.0,  1.0,
			 size, -size, -size,  _w,  1.0,  0.0,  0.0,  1.0,
			 size, -size,  size,  _w,  1.0,  0.0,  1.0,  1.0,
		};

		GLuint vertexPositionBuffer;
		glGenVertexArrays(1, &_boxArray); // generate array
		glBindVertexArray(_boxArray); // bind array
		glGenBuffers(1, &vertexPositionBuffer); // generate buffer
		glBindBuffer(GL_ARRAY_BUFFER, vertexPositionBuffer); // bind buffer
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, reinterpret_cast<void*>(0));
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, reinterpret_cast<void*>(sizeof(GLfloat) * 4));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		_fbo = new FramebufferObject();
		_fbo->activate();
		int x1, xSize, y1, ySize;
		sgct::Engine::instance()->getActiveWindowPtr()->getCurrentViewportPixelCoords(x1, y1, xSize, ySize);
		size_t x = static_cast<size_t>(xSize);
		size_t y = static_cast<size_t>(ySize);

		const ghoul::opengl::Texture::Format format = ghoul::opengl::Texture::Format::RGBA;
		GLint internalFormat = GL_RGBA;
		GLenum dataType = GL_FLOAT;

		_backTexture = new Texture(glm::size3_t(x, y, 1));
		_frontTexture = new Texture(glm::size3_t(x, y, 1));
		_outputTexture = new Texture(glm::size3_t(x, y, 1), format, GL_RGBA32F, dataType);
		_backTexture->uploadTexture();
		_frontTexture->uploadTexture();
		_outputTexture->uploadTexture();
		_fbo->attachTexture(_backTexture, GL_COLOR_ATTACHMENT0);
		_fbo->attachTexture(_frontTexture, GL_COLOR_ATTACHMENT1);
		_fbo->deactivate();

		if (_transferFunction)
			_transferFunction->uploadTexture();
		/*
		glGenTextures(3, _textures);
		glBindTexture(GL_TEXTURE_2D, _textures[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, _textures[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, _textures[2]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		*/
		
	}

	return success;
}

bool RenderableFlare::deinitialize() {
	return true;
}

void RenderableFlare::render(const RenderData& data) {
	GLuint activeFBO = FramebufferObject::getActiveObject(); // Save SGCTs main FBO
	_fbo->activate();
	_cubeProgram->activate();

	const Camera& camera = data.camera;
	const psc& position = data.position;
	setPscUniforms(_cubeProgram, &camera, position);
	_cubeProgram->setUniform("modelViewProjection", camera.viewProjectionMatrix());
	_cubeProgram->setUniform("modelTransform", glm::mat4(1.0));

	sgct_core::Frustum::FrustumMode mode = sgct::Engine::instance()->
		getActiveWindowPtr()->
		getCurrentViewport()->
		getEye();

	// oh god why..?
	if (mode == sgct_core::Frustum::FrustumMode::Mono ||
		mode == sgct_core::Frustum::FrustumMode::StereoLeftEye) {
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);

	}
	// make sure GL_CULL_FACE is enabled (it should be)
	glEnable(GL_CULL_FACE);

	//      Draw backface
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glCullFace(GL_FRONT);
	glBindVertexArray(_boxArray);
	glDrawArrays(GL_TRIANGLES, 0, 6 * 6);
	//      Draw frontface (now the normal cull face is is set)
	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	glCullFace(GL_BACK);
	glDrawArrays(GL_TRIANGLES, 0, 6 * 6);
	_cubeProgram->deactivate();
	_fbo->deactivate();

	// rebind the previous FBO
	glBindFramebuffer(GL_FRAMEBUFFER, activeFBO);

	// Prepare positional data
	//const Camera& camera = data.camera;
	//const psc& position = data.position;
	//setPscUniforms(_tspTraversal, &camera, position);


	// Dispatch TSP traversal
	launchTSPTraversal(0);
	

	
	/*
	glTextureBarrierNV();
	
	
	GLsync syncObject = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	GLenum ret = glClientWaitSync(syncObject, GL_SYNC_FLUSH_COMMANDS_BIT, 1000 * 1000 * 1000);
	if (ret == GL_WAIT_FAILED || ret == GL_TIMEOUT_EXPIRED)
		LERROR("glClientWaitSync failed.");
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glDeleteSync(syncObject);
	

	
	glFlush();
	glFinish();
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	*/
	// PBO to atlas

	// Dispatch Raycaster
	//_tspTraversal->activate();
	//glDispatchComputeIndirect(0);
	//_tspTraversal->deactivate();


	// Disk to PBO

	//glMemoryBarrier(GL_ALL_BARRIER_BITS);
	// To screen
	
	_textureToAbuffer->activate();
	//glBindImageTexture(5, *_backTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	setPscUniforms(_textureToAbuffer, &camera, position);
	_textureToAbuffer->setUniform("modelViewProjection", camera.viewProjectionMatrix());
	_textureToAbuffer->setUniform("modelTransform", glm::mat4(1.0));

	//i = _textureToAbuffer->uniformLocation("reqList");
	//LDEBUG(i);
	//glBindImageTexture(5, *_backTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

	// Bind texture
	ghoul::opengl::TextureUnit unit;
	unit.activate();
	_outputTexture->bind();
	//glBindTexture(GL_TEXTURE_2D, _textures[2]);
	//_frontTexture->bind();
	//_backTexture->bind();
	_textureToAbuffer->setUniform("texture1", unit);

	glBindVertexArray(_boxArray);
	glDrawArrays(GL_TRIANGLES, 0, 6 * 6);
	_textureToAbuffer->deactivate();
	glDisable(GL_CULL_FACE);
	glBindImageTexture(5, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);


}

void RenderableFlare::update(const UpdateData& data) {
}

void RenderableFlare::launchTSPTraversal(int timestep){
	_tspTraversal->activate();
	ghoul::opengl::TextureUnit unit1;
	ghoul::opengl::TextureUnit unit2;
	//ghoul::opengl::TextureUnit unit3;

	unit1.activate();
	_frontTexture->bind();
	_tspTraversal->setUniform("cubeFront", unit1);

	unit2.activate();
	_backTexture->bind();
	_tspTraversal->setUniform("cubeBack", unit2);

	//unit3.activate();
	//_outputTexture->bind();
	//_textureToAbuffer->setUniform("reqList", unit3);
	//glBindImageTexture(3, *_outputTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	//glBindImageTexture(3, *_frontTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	//glBindImageTexture(4, *_backTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
	//glBindImageTexture(5, *_outputTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	/*
	glFlush();
	glFinish();
	glTextureBarrierNV();
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	*/

	//glBindImageTexture(i, _textures[2], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	//glBindImageTexture(i, *_outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	GLint i = _tspTraversal->uniformLocation("reqList");
	//LDEBUG(i);
	glBindImageTexture(3, *_outputTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glDispatchComputeIndirect(0);
	//glDispatchCompute(1280 / 16, 720 / 16, 1);
	//glFinish();
	//glBindImageTexture(2, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	_tspTraversal->deactivate();
}
void RenderableFlare::PBOToAtlas(size_t buffer){
}
void RenderableFlare::buildBrickList(size_t buffer, const Bricks& bricks){
}
void RenderableFlare::diskToPBO(size_t buffer){
}


} // namespace openspace
