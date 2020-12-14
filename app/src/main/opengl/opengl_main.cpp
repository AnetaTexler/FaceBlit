#include <opengl_main.hpp>

GlobalOpenglData globalOpenglData = GlobalOpenglData(); // Init OpenGL global data structure.

void FB_OpenGL::opengl_debug() {
	std::cout << "All okay!" << std::endl;
}

int initSDL() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); // Enable to tell SDL that the old, deprecated GL code are disabled, only the newer versions can be used.
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	globalOpenglData.mainWindow = SDL_CreateWindow("Hello World", globalOpenglData.w_pos_x, globalOpenglData.w_pos_y, globalOpenglData.w_width, globalOpenglData.w_height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (globalOpenglData.mainWindow == NULL) {
		SDL_Log("Unable to create SDL Window: %s", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	globalOpenglData.mainContext = SDL_GL_CreateContext(globalOpenglData.mainWindow);
	if (globalOpenglData.mainContext == NULL) {
		SDL_Log("Unable to create GL Context: %s", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	if (SDL_GL_SetSwapInterval(1) < 0) // Configure VSync.
	{
		SDL_Log("Warning: Unable to set VSync!: %s", SDL_GetError());
		return -1;
	}
	std::cout << "SDL2 init OK!" << std::endl;

	glewExperimental = GL_TRUE;		// Enable to tell OpenGL that we want to use OpenGL 3.0 stuff and later.
	GLenum nGlewError = glewInit(); // Initialize glew.
	if (nGlewError != GLEW_OK)
	{
		SDL_Log("Error initializing GLEW! %s", glewGetErrorString(nGlewError));
		return -1;
	}
	else { std::cout << "Glew init OK!" << std::endl; }

	// std::string strWindowTitle = "";
	// SDL_SetWindowTitle(globalOpenglData.mainWindow, strWindowTitle.c_str());

	return 0;
}

int initOpenGL() {

	int success = 0;
	GLenum error = GL_NO_ERROR;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if ((error = glGetError()) != GL_NO_ERROR)
	{
		std::cout << "Error initializing OpenGL!" << gluErrorString(error) << std::endl;
		return -1;
	}

	glViewport(0, 0, globalOpenglData.w_width, globalOpenglData.w_height);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	std::cout << "OpenGL init OK!" << std::endl;

	return success;
}

bool FB_OpenGL::init() {
	globalOpenglData.w_width = 728; globalOpenglData.w_height = 1024;
	globalOpenglData.w_pos_x = 100; globalOpenglData.w_pos_y = 100;

	globalOpenglData.P = glm::perspective(glm::radians(60.0f), float(globalOpenglData.w_width) / float(globalOpenglData.w_height), 0.1f, 100.f);
	globalOpenglData.V = glm::lookAt(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-3.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	if (initSDL() == -1) return -1;
	if (initOpenGL() == -1) return -1;

	return true;
}

bool FB_OpenGL::SetOpenGLAttributes()
{
	// Set our OpenGL version.
	// SDL_GL_CONTEXT_CORE gives us only the newer version, deprecated functions are disabled.
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// 3.2, just to be safe.
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

	// Turn on double buffering with a 24bit Z buffer.
	// You may need to change this to 16 or 32 for your system.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	return true;
}

FB_OpenGL::FullScreenQuad::FullScreenQuad(Shader* _shader) {
	shader = _shader;

	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);

	/*for (size_t i = 0; i < 16; i++)
	{
		std::cout << coords[i] << " ";
	}
	std::cout << std::endl;*/
	//std::cout << sizeof(coords) << " vs " << size << std::endl;

	glGenBuffers(1, &vertexBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screenCoords), screenCoords, GL_STATIC_DRAW);

	glEnableVertexAttribArray(_shader->getPosLocation());
	glVertexAttribPointer(_shader->getPosLocation(), 2, GL_FLOAT, GL_FALSE, 0, 0);

	un_short = false;

	glBindVertexArray(0);
	glUseProgram(0);
}

FB_OpenGL::Grid::Grid(int x_count, int y_count, float x_min, float x_max, float y_min, float y_max, Shader* _shader) {
	shader = _shader;

	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);

	/*float x_max = 1.0f;
	float x_min = 0.0f;
	float y_max = 1.0f;
	float y_min = 0.0f;*/

	float x_spacing = (x_max - x_min) / float(x_count - 1);
	float y_spacing = (y_max - y_min) / float(y_count - 1);

	std::vector<float> vertices_local;
	std::vector<float> vertices_uv_local;
	std::vector<unsigned short> triangle_indices;

	for (int y = 0; y < y_count; y++)
	{
		for (int x = 0; x < x_count; x++)
		{
			vertices_local.push_back((x * x_spacing + x_min)* 2.0f - 1.0f);
			vertices_local.push_back((y * y_spacing + y_min)* 2.0f - 1.0f);

			vertices_uv_local.push_back(x * x_spacing + x_min);
			vertices_uv_local.push_back(y * y_spacing + y_min);

			VertexDeformInfo info;
			info.x = (x * x_spacing + x_min) * 2.0f - 1.0f;
			info.y = (y * y_spacing + y_min) * 2.0f - 1.0f;
			vertexDeformations.push_back(info);
		}
	}

	numTriangles = 0;
	for (int y = 0; y < y_count-1; y++) // for every grid row.
	{
		for (int x = 0; x < x_count-1; x++) // for every column.
		{
			triangle_indices.push_back(x + x_count*y);
			triangle_indices.push_back(x + 1 + x_count * y);
			triangle_indices.push_back(x + x_count + x_count * y);

			triangle_indices.push_back(x + 1 + x_count * y);
			triangle_indices.push_back(x + x_count + x_count * y);
			triangle_indices.push_back(x + x_count + 1 + x_count * y);

			numTriangles += 2;
		}
	}

	glGenBuffers(1, &vertexBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, vertices_local.size() * sizeof(float), &vertices_local[0], GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(_shader->getPosLocation());
	glVertexAttribPointer(_shader->getPosLocation(), 2, GL_FLOAT, GL_FALSE, 0, 0);

	vertices = vertices_local;
	vertices_rest = vertices_local;
	triangle_ids = triangle_indices;
	un_short = false;

	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices_uv_local.size() * sizeof(float), &vertices_uv_local[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(_shader->getTexCoordLocation());
	glVertexAttribPointer(_shader->getTexCoordLocation(), 2, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &elementBufferObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangle_indices.size() * sizeof(unsigned short), &triangle_indices[0], GL_STATIC_DRAW);


	glBindVertexArray(0);
	glUseProgram(0);
}

void FB_OpenGL::FullScreenQuad::draw() {
	shader->useProgram();

	glBindVertexArray(vertexArrayObject);

	// Texture handling
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *(textureID));
	glUniform1i(shader->getTexSamplerLocation(), 0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	glUseProgram(0);
}

void FB_OpenGL::Grid::draw() {
	shader->useProgram();

	glBindVertexArray(vertexArrayObject);

	// Update vertex data.
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_DYNAMIC_DRAW);
	/*glEnableVertexAttribArray(shader->getPosLocation());
	glVertexAttribPointer(shader->getPosLocation(), 2, GL_FLOAT, GL_FALSE, 0, 0);*/

	// Texture handling.
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *(textureID));
	glUniform1i(shader->getTexSamplerLocation(), 0);

	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	// glLineWidth(3);
	glDrawElements(GL_TRIANGLES, numTriangles * 3, GL_UNSIGNED_SHORT, 0);
	// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBindVertexArray(0);

	glUseProgram(0);
}

void FB_OpenGL::StyblitRenderer::draw() {
	shader->useProgram();

	glBindVertexArray(vertexArrayObject);

	// Texture handling
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *(stylePosGuideTextureID));
	glUniform1i(shader->getStylePosGuideLocation(), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, *(targetPosGuideTextureID));
	glUniform1i(shader->getTargetPosGuideLocation(), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, *(styleAppGuideTextureID));
	glUniform1i(shader->getStyleAppGuideLocation(), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, *(targetAppGuideTextureID));
	glUniform1i(shader->getTargetAppGuideLocation(), 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, *(styleImgTextureID));
	glUniform1i(shader->getStyleImgLocation(), 4);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_3D, *(LUTTextureID));
	glUniform1i(shader->getLUTLocation(), 5);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, *(jitterTableID));
	glUniform1i(shader->getJitterTableLocation(), 6);

	glUniform1i(shader->getWidthLocation(), width);
	glUniform1i(shader->getHeightLocation(), height);
	glUniform1f(shader->getThresholdLocation(), threshold);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	glUseProgram(0);
}

void FB_OpenGL::StyblitBlender::draw() {
	shader->useProgram();

	glBindVertexArray(vertexArrayObject);

	// Texture handling
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *(NNFID));
	glUniform1i(shader->getNNFLocation(), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, *(styleImgID));
	glUniform1i(shader->getStyleImgLocation(), 1);

	glUniform1i(shader->getWidthLocation(), width);
	glUniform1i(shader->getHeightLocation(), height);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	glUseProgram(0);
}

const char* GetGLErrorStr(GLenum err)
{
	switch (err)
	{
	case GL_NO_ERROR:          return "No error";
	case GL_INVALID_ENUM:      return "Invalid enum";
	case GL_INVALID_VALUE:     return "Invalid value";
	case GL_INVALID_OPERATION: return "Invalid operation";
	case GL_STACK_OVERFLOW:    return "Stack overflow";
	case GL_STACK_UNDERFLOW:   return "Stack underflow";
	case GL_OUT_OF_MEMORY:     return "Out of memory";
	default:                   return "Unknown error";
	}
}

GLuint FB_OpenGL::makeTexture(cv::Mat image) {
	GLuint texture;

	// glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	cv::flip(image, image, 0); // OpenCV stores top to bottom, but we need the image bottom to top for OpenGL.
	cv::cvtColor(image, image, cv::COLOR_BGR2RGB); // OpenCV uses BGR format, need to convert it to RGB for OpenGL.

	if (image.type() == CV_32FC3) {
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGB32F,
			image.cols,
			image.rows,
			0,
			GL_RGB,
			GL_FLOAT,
			image.ptr());
	}
	else if (image.type() == CV_16UC3) {
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGB16,
			image.cols,
			image.rows,
			0,
			GL_RGB,
			GL_UNSIGNED_SHORT,
			image.ptr());
	} else {
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGB8,
			image.cols,
			image.rows,
			0,
			GL_RGB,
			GL_UNSIGNED_BYTE,
			image.ptr());
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	const GLenum err = glGetError();
	if (GL_NO_ERROR != err)
		std::cout << "GL Error: " << GetGLErrorStr(err) << std::endl;

	return texture;
}

GLuint FB_OpenGL::makeJitterTable(cv::Mat image) {
	GLuint texture;

	// glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	cv::flip(image, image, 0); // OpenCV stores top to bottom, but we need the image bottom to top for OpenGL.
	//cv::cvtColor(image, image, cv::COLOR_BGR2RGB); // OpenCV uses BGR format, need to convert it to RGB for OpenGL.

	glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RG,
		image.cols,
		image.rows,
		0,
		GL_RG,
		GL_UNSIGNED_BYTE,
		image.ptr());

	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

GLuint FB_OpenGL::make3DTexture(cv::Mat cube) {
	GLuint texture;

	// glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_3D, texture);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//cv::flip(image, image, 0); // OpenCV stores top to bottom, but we need the image bottom to top for OpenGL.
	//cv::cvtColor(image, image, cv::COLOR_BGR2RGB); // OpenCV uses BGR format, need to convert it to RGB for OpenGL.

	glTexImage3D(GL_TEXTURE_3D,
		0,
		GL_RG16UI,
		cube.size[0],
		cube.size[1],
		cube.size[2],
		0,
		GL_RG_INTEGER,
		GL_UNSIGNED_SHORT,
		cube.ptr());

	glBindTexture(GL_TEXTURE_3D, 0);

	return texture;
}

void FB_OpenGL::updateTexture(GLint texture, cv::Mat image) {

	glBindTexture(GL_TEXTURE_2D, texture);

	cv::flip(image, image, 0); // OpenCV stores top to bottom, but we need the image bottom to top for OpenGL.
	cv::cvtColor(image, image, cv::COLOR_BGR2RGB); // OpenCV uses BGR format, need to convert it to RGB for OpenGL.

	if (image.type() == CV_32FC3) {
		glTexSubImage2D(GL_TEXTURE_2D,
			0,
			0,
			0,
			image.cols,
			image.rows,
			GL_RGB,
			GL_FLOAT,
			image.ptr());
	}
	else if (image.type() == CV_16UC3) {
		glTexSubImage2D(GL_TEXTURE_2D,
			0,
			0,
			0,
			image.cols,
			image.rows,
			GL_RGB,
			GL_UNSIGNED_SHORT,
			image.ptr());
	}
	else {
		glTexSubImage2D(GL_TEXTURE_2D,
			0,
			0,
			0,
			image.cols,
			image.rows,
			GL_RGB,
			GL_UNSIGNED_BYTE,
			image.ptr());
	}

	glBindTexture(GL_TEXTURE_2D, 0);

}

bool FB_OpenGL::makeFrameBuffer(GLsizei width, GLsizei height, GLuint& frame_buffer, GLuint& render_buffer_depth_stencil, GLuint& tex_color_buffer) {
	// Create and bind the frame buffer.
	glGenFramebuffers(1, &frame_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

	// Create texture where we are going to bulk the contents of the frame buffer.
	glGenTextures(1, &tex_color_buffer);
	glBindTexture(GL_TEXTURE_2D, tex_color_buffer);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Attach the image to the framebuffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_color_buffer, 0); // The second parameter implies that you can have multiple color attachments. A fragment shader can output
																									  // different data to any of these by linking 'out' variables to attachments with the glBindFragDataLocation function.

	// Create the render buffer to host the depth and stencil buffers.
	// Although we could do this by creating another texture, it is more efficient to store these buffers in a Renderbuffer Object, because we're only interested in reading the color buffer in a shader.
	glGenRenderbuffers(1, &render_buffer_depth_stencil);
	glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_depth_stencil);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	// Attach the render buffer to the framebuffer
	// NOTE: Even if you don't plan on reading from this depth_attachment, an off screen buffer that will be rendered to should have a depth attachment
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, render_buffer_depth_stencil);

	// Check whether the frame buffer is complete (at least one buffer attached, all attachmentes are complete, all attachments same number multisamples)
	(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) ? std::cout << "The frame buffer is complete!" << std::endl : std::cout << "The frame buffer is invalid, please re-check. Code: " << glCheckFramebufferStatus(frame_buffer) << std::endl;

	return true;
}

cv::Mat FB_OpenGL::get_ocv_img_from_gl_img(GLuint ogl_texture_id)
{
	glBindTexture(GL_TEXTURE_2D, ogl_texture_id);
	GLenum gl_texture_width, gl_texture_height;

	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, (GLint*)&gl_texture_width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, (GLint*)&gl_texture_height);

	unsigned char* gl_texture_bytes = (unsigned char*)malloc(sizeof(unsigned char) * gl_texture_width * gl_texture_height * 3);
	glGetTexImage(GL_TEXTURE_2D, 0 /* mipmap level */, GL_BGR, GL_UNSIGNED_BYTE, gl_texture_bytes);

	cv::Mat image = cv::Mat(gl_texture_height, gl_texture_width, CV_8UC3, gl_texture_bytes);
	cv::flip(image, image, 0); // OpenCV stores top to bottom, but we need the image bottom to top for OpenGL.

	return image;
}


// *************************** DEFORMATIONS ************************************************

int FB_OpenGL::Grid::getNearestControlPointID(float x, float y) {
	int best_id = 0;
	float best_d = 420.0f;
	
	for (size_t i = 0; i < vertices.size()/2; i++)
	{
		float dist = sqrt( pow( x - vertices[2*i] ,2.0f) + pow(y - vertices[2 * i + 1],2.0f) );
		if (dist < best_d) {
			best_d = dist; best_id = i;
		}
	}

	return best_id;
}

void FB_OpenGL::Grid::setPointCoordinates(int id, float x, float y) {
	vertexDeformations[id].fixed = true;
	vertexDeformations[id].x = x;
	vertexDeformations[id].y = y;
}

std::vector<int> FB_OpenGL::Grid::getQuadFromTriangleID(int triangle_id) {
	std::vector<int> ids;

	if (triangle_id % 2 == 1) triangle_id--;

	ids.push_back(triangle_ids[triangle_id * 3]);
	ids.push_back(triangle_ids[triangle_id * 3 + 1]);
	ids.push_back(triangle_ids[triangle_id * 3 + 2]);
	ids.push_back(triangle_ids[triangle_id * 3 + 5]);

	return ids;
}

void FB_OpenGL::Grid::deformGrid(int iterations) {
	int i, j, k;
	std::vector<bool> active;

	for (i = 0; i < numTriangles; i = i+2) {
		bool current = false;
		std::vector<int> vertex_ids = getQuadFromTriangleID(i);
		for (j = 0; j < 4; j++) if (vertexDeformations[vertex_ids[j]].fixed) { current = true; break; }
		active.push_back(current);
	}

	for (k = 0; k < iterations; k++) {
		// null the vertices array
		// init and null the weights array
		weights.clear();

		for (i = 0; i < vertices.size() / 2; i++) {
			weights.push_back(0.0f);

			if (vertexDeformations[i].fixed) continue;

			vertices[2 * i] = 0.0f;
			vertices[2 * i + 1] = 0.0f;

		}

		for (i = 0; i < numTriangles; i = i + 2) {

			if (active[i/2]) activeSquareFit(i); else passiveSquareFit(i);

		}

		for (i = 0; i < vertices.size() / 2; i++) {
			if (vertexDeformations[i].fixed) continue;

			vertices[2 * i] /= weights[i];
			vertices[2 * i + 1] /= weights[i];
			vertexDeformations[i].x = vertices[2 * i];
			vertexDeformations[i].y = vertices[2 * i + 1];

		}
	}
}


#define SQR(A) ((A)*(A))
#define ZERO 1e-6
#define CP 4096

void FB_OpenGL::Grid::activeSquareFit(int triangle_id) {
	int i; double sum, w, r0, r1, Tx, Ty, sx, sy, tx, ty, csx = 0, csy = 0, ctx = 0, cty = 0, cw = 0;

	std::vector<int> vertex_ids = getQuadFromTriangleID(triangle_id);

	for (i = 0; i < 4; i++) {
		int vert_id = vertex_ids[i];
		w = vertexDeformations[vert_id].fixed ? CP : 1;

		csx += vertices_rest[2 * vert_id] * w;
		csy += vertices_rest[2 * vert_id + 1] * w;
		ctx += vertexDeformations[vert_id].x * w;
		cty += vertexDeformations[vert_id].y * w;

		cw += w;

	}

	w = 1. / cw; sum = 0;

	csx *= w; ctx *= w;
	csy *= w; cty *= w;

	r0 = 0; r1 = 0;

	for (i = 0; i < 4; i++) {
		int vert_id = vertex_ids[i];
		w = vertexDeformations[vert_id].fixed ? CP : 1;

		sx = vertices_rest[2 * vert_id] - csx;
		sy = vertices_rest[2 * vert_id + 1] - csy;
		tx = vertexDeformations[vert_id].x - ctx;
		ty = vertexDeformations[vert_id].y - cty;

		r0 += w * (sx * tx + sy * ty);
		r1 += w * (sy * tx - sx * ty);
	}

	w = sqrt(SQR(r0) + SQR(r1));

	if (fabs(w) < ZERO) return;

	w = 1. / w; r0 *= w; r1 *= w;

	Tx = ctx - r0 * csx - r1 * csy;
	Ty = cty + r1 * csx - r0 * csy;

	for (i = 0; i < 4; i++) {
		/*v = q->V + i; 
		p = q->P[i];

		v->x = Tx + r0 * p->sx + r1 * p->sy;
		v->y = Ty - r1 * p->sx + r0 * p->sy;*/

		int vert_id = vertex_ids[i];

		if (vertexDeformations[vert_id].fixed) {
			vertices[2 * vert_id] = Tx + r0 * vertices_rest[2 * vert_id] + r1 * vertices_rest[2 * vert_id + 1];
			vertices[2 * vert_id + 1] = Ty - r1 * vertices_rest[2 * vert_id] + r0 * vertices_rest[2 * vert_id + 1];
		} else {
			vertices[2 * vert_id] += Tx + r0 * vertices_rest[2 * vert_id] + r1 * vertices_rest[2 * vert_id + 1];
			vertices[2 * vert_id + 1] += Ty - r1 * vertices_rest[2 * vert_id] + r0 * vertices_rest[2 * vert_id + 1];
			weights[vert_id]++;
		}

	}
}

void FB_OpenGL::Grid::passiveSquareFit(int triangle_id) {
	int i; 
	// POINT* p; VERTEX* v; 
	double sum, w, r0, r1, Tx, Ty, tx, ty, ctx = 0, cty = 0, csx = 0, csy = 0;
	std::vector<int> vertex_ids = getQuadFromTriangleID(triangle_id);

	for (i = 0; i < 4; i++) { 
		int vert_id = vertex_ids[i];
		csx += vertices_rest[2 * vert_id];
		csy += vertices_rest[2 * vert_id + 1];
		ctx += vertexDeformations[vert_id].x;
		cty += vertexDeformations[vert_id].y;
	} 
	ctx *= 0.25; 
	cty *= 0.25; 
	csx *= 0.25;
	csy *= 0.25;
	r0 = 0; 
	r1 = 0; 
	sum = 0;

	for (i = 0; i < 4; i++) {
		int vert_id = vertex_ids[i];

		tx = vertexDeformations[vert_id].x - ctx;
		ty = vertexDeformations[vert_id].y - cty;

		r0 += (vertices_rest[2 * vert_id] - csx) * tx + (vertices_rest[2 * vert_id + 1] - csy) * ty;
		r1 += (vertices_rest[2 * vert_id + 1] - csy) * tx - (vertices_rest[2 * vert_id] - csx) * ty;

	}

	w = sqrt(SQR(r0) + SQR(r1));

	if (fabs(w) < ZERO) return;

	w = 1. / w; r0 *= w; r1 *= w;

	Tx = ctx - r0 * csx - r1 * csy;
	Ty = cty + r1 * csx - r0 * csy;

	for (i = 0; i < 4; i++) {
		int vert_id = vertex_ids[i];

		vertices[2 * vert_id] += Tx + r0 * vertices_rest[2 * vert_id] + r1 * vertices_rest[2 * vert_id + 1];
		vertices[2 * vert_id + 1] += Ty - r1 * vertices_rest[2 * vert_id] + r0 * vertices_rest[2 * vert_id + 1];
		weights[vert_id]++;

	}
}
