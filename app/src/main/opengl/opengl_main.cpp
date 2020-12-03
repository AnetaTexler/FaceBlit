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