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

			GLint getWidthLocation() { return widthLocation; }
			GLint getHeightLocation() { return heightLocation; }

			GLint getStylePosGuideLocation() { return stylePosGuideLocation; }
			GLint getTargetPosGuideLocation() { return targetPosGuideLocation; }
			GLint getStyleAppGuideLocation() { return styleAppGuideLocation; }
			GLint getTargetAppGuideLocation() { return targetAppGuideLocation; }
			GLint getStyleImgLocation() { return styleImgLocation; }
			GLint getLUTLocation() { return LUTLocation; }
			GLint getJitterTableLocation() { return jitterTableLocation; }
			GLint getThresholdLocation() { return thresholdLocation; }
			GLint getNNFLocation() { return NNFLocation;}
			GLint getALocation() { return ALocation;}
			GLint getBLocation() { return BLocation;}
			GLint getMaskLocation() { return maskLocation;}

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

			GLint widthLocation;
			GLint heightLocation;

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

			GLint LUTLocation;
			GLint jitterTableLocation;
			GLint thresholdLocation;
			GLint NNFLocation;
			GLint ALocation;
			GLint BLocation;
			GLint maskLocation;
	};

};

#endif // _FACEBLIT_SHADER

