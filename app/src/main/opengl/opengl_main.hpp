#ifndef _FACEBLIT_OPENGL
#define _FACEBLIT_OPENGL

#define SDL_MAIN_HANDLED // SDL2 has its own "main" function that would cause collision with our main during compilation.

#include <iostream>
#include <SDL2/SDL.h>
#include <gl/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <shader.hpp>

struct GlobalOpenglData {
	std::string programName;

	// SDL_Window.
	SDL_Window* mainWindow;

	// Opengl context handle.
	SDL_GLContext mainContext;
	int w_width, w_height, w_pos_x, w_pos_y;

	// Transforms.
	glm::mat4 P, V;
};
extern GlobalOpenglData globalOpenglData;

const float screenCoords[] = {
		-1.0f, -1.0f,
		1.0f, -1.0f,
		-1.0f,  1.0f,
		1.0f,  1.0f
};

namespace FB_OpenGL {

	bool init();
	bool SetOpenGLAttributes();
	void opengl_debug();


	class FullScreenQuad {
	public:
		FullScreenQuad() {}
		FullScreenQuad(Shader* _shader);
		~FullScreenQuad() {}

		void setTextureID(GLuint *_textureID) { textureID = _textureID; }
		void draw();

	protected:
		Shader* shader = NULL;

		GLuint        vertexBufferObject;
		GLuint        elementBufferObject;
		GLuint        vertexArrayObject;
		unsigned int  numTriangles;

		glm::vec3     ambient;
		glm::vec3     diffuse;
		glm::vec3     specular;
		float         shininess;

		GLuint        texture;
		bool		  hasTexture;

		bool un_short;

		GLuint* textureID = NULL;

	};

};

#endif // _FACEBLIT_OPENGL