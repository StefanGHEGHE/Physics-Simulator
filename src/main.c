#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "glad/glad.h"
#include "glfw/glfw3.h"
#include "cglm/cglm.h"

typedef struct { 
    vec3 position;
    vec3 velocity; 
} Particle;

char *read_shader_source(const char *filepath)
{
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open shader file: %s\n", filepath);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *source = malloc((length + 1) * sizeof(char));
    if (source) {
        fread(source, 1, length * sizeof(char), file);
        source[length] = '\0';
        fclose(file);
        return source;
    } else {
        fprintf(stderr, "Failed to allocate memory for shader source\n");
        fclose(file);
        return NULL;
    }
}

GLuint load_and_compile_shader(const char *filepath, GLenum shaderType)
{
    char *source = read_shader_source(filepath);
    if (!source) {
        return 0;
    }
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, (const char**)&source, NULL);
    glCompileShader(shader);
    free(source);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "Error compiling shader %s: %s\n", filepath, infoLog);
        return 0;
    }
    return shader;
}

GLuint attach_shaders(GLuint shader_count, ...)
{
	va_list shaders;
	va_start(shaders, shader_count);
	GLuint shaderProgram = glCreateProgram();
	for (int i = 0; i < shader_count; i++) {
		GLuint shader = va_arg(shaders, GLuint);
		glAttachShader(shaderProgram, shader);
	}
	va_end(shaders);
	glLinkProgram(shaderProgram);

	GLint success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		fprintf(stderr, "Error linking shader program: %s\n", infoLog);
		glDeleteProgram(shaderProgram);
		return 0;
	}
	return shaderProgram;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

	// garbage code to test multiple monitor support
    int monitor_count;
    GLFWmonitor **monitors = glfwGetMonitors(&monitor_count);
    GLFWmonitor *primaryMonitor = (monitors && monitor_count > 1) ? monitors[1] : NULL;
    GLFWmonitor *secondaryMonitor = monitors ? monitors[0] : NULL;

	float vertices1[] = {
		-0.5f, -0.5f, 0.0f, // top right
		-0.25f, -0.5f, 0.0f, // bottom right
		-0.0f, -0.0f, 0.0f, // top middle
	};
	float vertices2[] = {
		0.5f, 0.5f, 0.0f, // top right
		0.25f, 0.5f, 0.0f, // bottom right
		0.0f, 0.0f, 0.0f, // bottom middle
	};
	unsigned int indices[] = { // note that we start from 0!
		0, 1, 2, // first triangle
		2, 3, 4 // second triangle
	};

    GLFWwindow* window = glfwCreateWindow(800, 600, "Physics Engine", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

	GLint success;
	GLchar infoLog[512];

    GLuint vertexShader = load_and_compile_shader("src/shaders/vertex.glsl", GL_VERTEX_SHADER);
    GLuint fragmentShader = load_and_compile_shader("src/shaders/fragment.glsl", GL_FRAGMENT_SHADER);

	GLuint shaderProgram = attach_shaders(2, vertexShader, fragmentShader);
	glUseProgram(shaderProgram);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	GLuint VBO[2], VAO[2];
	glGenVertexArrays(2, VAO);
	glGenBuffers(2, VBO);
	glBindVertexArray(VAO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices1), vertices1, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(VAO[1]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(shaderProgram);
		glBindVertexArray(VAO[0]);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindVertexArray(VAO[1]);
		glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
