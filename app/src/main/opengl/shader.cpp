#include <shader.hpp>

bool readShaderFile(std::string filename, std::string& out) {
	std::ifstream f(filename);

	if (!f.good()) {
		std::cout << "Cannot open shader file " << filename << std::endl;
		return false;
	}

	std::stringstream buffer;
	buffer << f.rdbuf();

	out = buffer.str();

	return true;
}

FB_OpenGL::Shader::Shader() {

};

FB_OpenGL::Shader::Shader(std::string vertexShader, std::string fragmentShader):vertexShaderFile(vertexShader), fragmentShaderFile(fragmentShader) {

};

FB_OpenGL::Shader::~Shader() {
	
};

bool FB_OpenGL::Shader::init() {
	if (vertexShaderFile.empty() || fragmentShaderFile.empty()) {
		std::cout << "ERROR: No shaders to init" << std::endl;
		return false;
	} else {
		program = glCreateProgram();

		/*glBindAttribLocation(program, 0, "in_Position");
		glBindAttribLocation(program, 1, "in_Normal");
		glBindAttribLocation(program, 2, "in_Texcoord");*/

		//std::cout << "Bind OK" << std::endl;
	}

	std::string shader_data;
	int compiled = 0;

	// VERTEX SHADER
	{
		vertexShader = glCreateShader(GL_VERTEX_SHADER);

		
		if (readShaderFile(vertexShaderFile, shader_data)) {
			GLint vs_len = shader_data.length();
			const char* vs_data = shader_data.data();
			glShaderSource(vertexShader, 1, &vs_data, &vs_len);

			// Compile the vertex shader
			glCompileShader(vertexShader);

			// Ask OpenGL if the shaders was compiled
			glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compiled);
			if (compiled == GL_FALSE) {

				GLint logSize = 0;
				glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logSize);
				// The maxLength includes the NULL character
				std::vector<GLchar> errorLog(logSize);
				glGetShaderInfoLog(vertexShader, logSize, &logSize, &errorLog[0]);


				std::cout << "There was a problem trying to compile the vertex shader" << std::endl;
				for (int i = 0; i < errorLog.size(); i++)
				{
					std::cout << errorLog[i];
				}

				return false;
			}

			// If all was fine with the creation and compilation, attach the vertex shader to the already created shader program
			glAttachShader(program, vertexShader);

			std::cout << "Vertex shader correctly generated and attached to shader program" << std::endl;
		}
	}

	// FRAGMENT SHADER
	{
		fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

		shader_data.clear();
		if (readShaderFile(fragmentShaderFile, shader_data)) {
			GLint fs_len = shader_data.length();
			const char* fs_data = shader_data.data();
			glShaderSource(fragmentShader, 1, &fs_data, &fs_len);

			// Compile the vertex shader
			glCompileShader(fragmentShader);

			// Ask OpenGL if the shaders was compiled
			compiled = 0;
			glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled);
			if (compiled == GL_FALSE) {
				GLint logSize = 0;
				glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logSize);
				// The maxLength includes the NULL character
				std::vector<GLchar> errorLog(logSize);
				glGetShaderInfoLog(vertexShader, logSize, &logSize, &errorLog[0]);

				std::cout << "There was a problem trying to compile the fragment shader" << std::endl;
				for (int i = 0; i < errorLog.size(); i++)
				{
					std::cout << errorLog[i];
				}
				return false;
			}

			// If all was fine with the creation and compilation, attach the vertex shader to the already created shader program
			glAttachShader(program, fragmentShader);
			glBindFragDataLocation(program, 0, "fragColor");

			std::cout << "Fragment shader correctly generated and attached to shader program" << std::endl;
		}
	}

	compiled = 0;

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &compiled);
	if (compiled == GL_FALSE) {
		std::cout << "There was a problem trying to assemble the shader program" << std::endl;
		return false;
	}

	posLocation = glGetAttribLocation(program, "in_Position");
	normalLocation = glGetAttribLocation(program, "in_Normal");
	texCoordLocation = glGetAttribLocation(program, "in_Texcoord");
	
	/*std::cout << "in_Position " << posLocation << std::endl;
	std::cout << "in_Normal " << normalLocation << std::endl;
	std::cout << "in_Texcoord " << texCoordLocation << std::endl;*/

	texSamplerLocation = glGetUniformLocation(program, "texSampler");

	PVMmatrixLocation = glGetUniformLocation(program, "PVMmatrix");
	MmatrixLocation = glGetUniformLocation(program, "Mmatrix");
	useTextureLocation = glGetUniformLocation(program, "useTexture");

	std::cout << "Shader init successful" << std::endl;
	return true;
};

bool FB_OpenGL::Shader::useProgram() {
	glUseProgram(program);

	return true;
};

/*
bool ViR2::Shader::setMVPmatrix(const glm::mat4);
bool ViR2::Shader::setModelMatrix(const glm::mat4);
bool ViR2::Shader::setViewMatrix(const glm::mat4);
bool ViR2::Shader::setProjectionMatrix(const glm::mat4);
*/