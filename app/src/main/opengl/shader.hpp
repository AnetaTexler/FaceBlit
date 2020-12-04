#ifndef _FACEBLIT_SHADER
#define _FACEBLIT_SHADER


#include <gl/glew.h>
#include <string>
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

namespace FB_OpenGL {

    class Shader {
        public:
            Shader();
            Shader(std::string vertexShader, std::string fragmentShader);
            ~Shader();

			bool init();
            bool useProgram();
			GLuint getShader() { return program; };

            /*bool setMVPmatrix(const glm::mat4);
            bool setModelMatrix(const glm::mat4);
            bool setViewMatrix(const glm::mat4);
            bool setProjectionMatrix(const glm::mat4);*/

			GLint getPosLocation() { return posLocation; }
			GLint getNormalLocation() { return normalLocation; }
			GLint getTexCoordLocation() { return texCoordLocation; }
			GLint getTexSamplerLocation() { return texSamplerLocation; }

			GLint getStylePosGuideLocation() { return stylePosGuideLocation; }
			GLint getTargetPosGuideLocation() { return targetPosGuideLocation; }
			GLint getStyleAppGuideLocation() { return styleAppGuideLocation; }
			GLint getTargetAppGuideLocation() { return targetAppGuideLocation; }
			GLint getStyleImgLocation() { return styleImgLocation; }

			GLuint getUseTextureLocation() { return useTextureLocation; }
			GLuint getPVMmatrixLocation() { return PVMmatrixLocation; }
			GLuint getMmatrixLocation() { return MmatrixLocation; }
			GLuint getVmatrixLocation() { return VmatrixLocation; }

        private:

            GLuint program;

            GLuint vertexShader;
            GLuint fragmentShader;
            
            std::string vertexShaderFile;
            std::string fragmentShaderFile;

			GLint posLocation;
			GLint normalLocation;
			GLint texCoordLocation;

			GLint texSamplerLocation;

			GLuint useTextureLocation;
			GLuint PVMmatrixLocation;
			GLuint VmatrixLocation;
			GLuint MmatrixLocation;

			GLint stylePosGuideLocation;
			GLint targetPosGuideLocation;
			GLint styleAppGuideLocation;
			GLint targetAppGuideLocation;
			GLint styleImgLocation;
	};

};

#endif // _FACEBLIT_SHADER

