#ifndef _FACEBLIT_OPENGL
#define _FACEBLIT_OPENGL

#define SDL_MAIN_HANDLED // SDL2 has its own "main" function that would cause collision with our main during compilation.

#include <iostream>
#include <SDL2/SDL.h>
#include <gl/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <shader.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

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

struct VertexDeformInfo {
	bool fixed = false;
	float x,y;
};

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

	GLuint makeTexture(cv::Mat image);
	GLuint makeJitterTable(cv::Mat image);
	void updateTexture(GLint texture, cv::Mat image);
	GLuint make3DTexture(cv::Mat cube);

	bool makeFrameBuffer(GLsizei width, GLsizei height, GLuint& frame_buffer, GLuint& render_buffer_depth_stencil, GLuint& tex_color_buffer);
	cv::Mat get_ocv_img_from_gl_img(GLuint ogl_texture_id);


	class FullScreenQuad {
	public:
		FullScreenQuad() {}
		FullScreenQuad(Shader* _shader);
		~FullScreenQuad() {}

		virtual void setTextureID(GLuint *_textureID) { textureID = _textureID; }
		virtual void draw();

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

	class Grid : public FullScreenQuad {
	public:
		Grid() {}
		Grid(int x_count, int y_count, float x_min, float x_max, float y_min, float y_max, Shader* _shader);
		~Grid() {}

		virtual void draw();

		GLuint uvbuffer;
		std::vector<float> vertices;
		std::vector<float> vertices_rest;
		std::vector<unsigned short> triangle_ids;
		std::vector<VertexDeformInfo> vertexDeformations;

		int getNearestControlPointID(float x, float y);
		std::vector<int> getQuadFromTriangleID(int triangle_id);
		void setPointCoordinates(int id, float x, float y);
		void deformGrid(int iterations);

	protected:

		std::vector<float> weights;

		void activeSquareFit(int triangle_id);
		void passiveSquareFit(int triangle_id);

	};

	class StyblitRenderer : public FullScreenQuad {
	public:
		StyblitRenderer() {}
		StyblitRenderer(Shader* _shader) : FullScreenQuad(_shader) {}
		~StyblitRenderer() {}

		virtual void draw();

		void setWidthHeight(int _width, int _height) { width = _width; height = _height; }

		void setTextures(GLuint* _stylePosGuideTextureID, 
			GLuint* _targetPosGuideTextureID, 
			GLuint* _styleAppGuideTextureID, 
			GLuint* _targetAppGuideTextureID, 
			GLuint* _styleImgTextureID,
			GLuint* _LUTTextureID) {
				stylePosGuideTextureID = _stylePosGuideTextureID;
				targetPosGuideTextureID = _targetPosGuideTextureID;
				styleAppGuideTextureID = _styleAppGuideTextureID;
				targetAppGuideTextureID = _targetAppGuideTextureID;
				styleImgTextureID = _styleImgTextureID;
				LUTTextureID = _LUTTextureID;
		}

		void setJitterTable(GLuint* _jitterTableID) {
			jitterTableID = _jitterTableID;
		}

		void incThreshold() { threshold++; std::cout << threshold << std::endl; };
		void decThreshold() { threshold--; std::cout << threshold << std::endl; };

	protected:
		GLuint* stylePosGuideTextureID = NULL;
		GLuint* targetPosGuideTextureID = NULL;
		GLuint* styleAppGuideTextureID = NULL;
		GLuint* targetAppGuideTextureID = NULL;
		GLuint* styleImgTextureID = NULL;
		GLuint* LUTTextureID = NULL;
		GLuint* jitterTableID = NULL;

		int width;
		int height;
		float threshold = 40.0f;
	};

	class StyblitBlender : public FullScreenQuad {
	public:
		StyblitBlender() {}
		StyblitBlender(Shader* _shader) : FullScreenQuad(_shader) {}
		~StyblitBlender() {}

		virtual void draw();

		void setWidthHeight(int _width, int _height) { width = _width; height = _height; }

		void setTextures(GLuint* _NNFID,
			GLuint* _styleImgID) {
			NNFID = _NNFID;
			styleImgID = _styleImgID;
		}

	protected:
		GLuint* NNFID = NULL;
		GLuint* styleImgID = NULL;

		int width;
		int height;
		
	};

	class Blending : public FullScreenQuad {
	public:
		Blending() {}
		Blending(Shader* _shader) : FullScreenQuad(_shader) {}
		~Blending() {}

		virtual void draw(GLuint A, GLuint B, GLuint mask, GLuint facialMask, GLuint background);

		void setWidthHeight(int _width, int _height) { width = _width; height = _height; }


	protected:
		int width;
		int height;

	};

};

#endif // _FACEBLIT_OPENGL