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
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

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