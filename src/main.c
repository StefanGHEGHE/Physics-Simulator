#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "glad/glad.h"
#include "glfw/glfw3.h"
#include "cglm/cglm.h"

#define G 0.5
#define DT 0.1
#define PI 3.1415926535

typedef struct { 
    vec3 position;
    vec3 velocity; 
} Particle;

typedef struct {
    double x, y, z;
    double vx, vy, vz;
    double mass, radius;
    float r, g, b; // Color for rendering
} Body;

// Central massive star
Body b1 = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 20000.0, 25.0, 1.0f, 0.8f, 0.1f};
// Planet orbiting the star
Body b2 = {150.0, 50.0, 100.0, -2.0, 4.0, 3.0, 10.0, 10.0, 0.2f, 0.6f, 1.0f};

// Camera state
float cameraRadius = 400.0f;
float cameraYaw = -90.0f; // -90 degrees points us directly at the origin along the Z axis
float cameraPitch = 0.0f;

// Mouse tracking state
int firstMouse = 1;
float lastX = 400.0f; // Center of an 800x600 window
float lastY = 300.0f;

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
        if (shader == 0) {
            fprintf(stderr, "Invalid shader id\n");
            continue; 
        }
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

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // Only rotate the camera if the right mouse button is held down
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS) {
        firstMouse = 1; // Reset so the camera doesn't jump on the next click
        return;
    }

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = 0;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed: y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.3f; // Adjust this to make rotation faster/slower
    cameraYaw += xoffset * sensitivity;
    cameraPitch += yoffset * sensitivity;

    // Constrain the pitch so the camera doesn't flip upside down
    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Adjust radius to zoom in and out
    cameraRadius -= (float)yoffset * 20.0f; 
    
    // Clamp the zoom so we don't go through the center or zoom out too far
    if (cameraRadius < 20.0f) cameraRadius = 20.0f;
    if (cameraRadius > 1500.0f) cameraRadius = 1500.0f;
}

// Physics update function in 3D
void updatePhysics() {
    double dx = b2.x - b1.x;
    double dy = b2.y - b1.y;
    double dz = b2.z - b1.z;
    double dist = sqrt(dx * dx + dy * dy + dz * dz);

    if (dist > (b1.radius + b2.radius) * 0.5) {
        double F = (G * b1.mass * b2.mass) / (dist * dist);
        
        double fx = F * (dx / dist);
        double fy = F * (dy / dist);
        double fz = F * (dz / dist);

        b1.vx += (fx / b1.mass) * DT;
        b1.vy += (fy / b1.mass) * DT;
        b1.vz += (fz / b1.mass) * DT;

        b2.vx -= (fx / b2.mass) * DT;
        b2.vy -= (fy / b2.mass) * DT;
        b2.vz -= (fz / b2.mass) * DT;

        b1.x += b1.vx * DT; b1.y += b1.vy * DT; b1.z += b1.vz * DT;
        b2.x += b2.vx * DT; b2.y += b2.vy * DT; b2.z += b2.vz * DT;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// Generate sphere vertices (positions + normals)
void generateSphere(float radius, int sectors, int stacks, float** vertices, int* vertexCount) {
    *vertexCount = sectors * stacks * 6; // 2 triangles per quad
    *vertices = (float*)malloc(*vertexCount * 6 * sizeof(float)); // 3 pos + 3 normal
    
    int idx = 0;
    for (int i = 0; i < stacks; ++i) {
        float phi1 = PI * (float)i / stacks;
        float phi2 = PI * (float)(i + 1) / stacks;
        for (int j = 0; j < sectors; ++j) {
            float theta1 = 2.0f * PI * (float)j / sectors;
            float theta2 = 2.0f * PI * (float)(j + 1) / sectors;

            // Compute 4 corners of the quad
            vec3 v1 = {sin(phi1)*cos(theta1), cos(phi1), sin(phi1)*sin(theta1)};
            vec3 v2 = {sin(phi2)*cos(theta1), cos(phi2), sin(phi2)*sin(theta1)};
            vec3 v3 = {sin(phi2)*cos(theta2), cos(phi2), sin(phi2)*sin(theta2)};
            vec3 v4 = {sin(phi1)*cos(theta2), cos(phi1), sin(phi1)*sin(theta2)};

            // Triangle 1 (v1, v2, v3)
            for(int k=0; k<3; k++) (*vertices)[idx++] = v1[k] * radius; // Pos
            for(int k=0; k<3; k++) (*vertices)[idx++] = v1[k];          // Norm
            for(int k=0; k<3; k++) (*vertices)[idx++] = v2[k] * radius;
            for(int k=0; k<3; k++) (*vertices)[idx++] = v2[k];
            for(int k=0; k<3; k++) (*vertices)[idx++] = v3[k] * radius;
            for(int k=0; k<3; k++) (*vertices)[idx++] = v3[k];

            // Triangle 2 (v1, v3, v4)
            for(int k=0; k<3; k++) (*vertices)[idx++] = v1[k] * radius;
            for(int k=0; k<3; k++) (*vertices)[idx++] = v1[k];
            for(int k=0; k<3; k++) (*vertices)[idx++] = v3[k] * radius;
            for(int k=0; k<3; k++) (*vertices)[idx++] = v3[k];
            for(int k=0; k<3; k++) (*vertices)[idx++] = v4[k] * radius;
            for(int k=0; k<3; k++) (*vertices)[idx++] = v4[k];
        }
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "3D Gravity Simulation", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // Enable Depth Testing for 3D
    glEnable(GL_DEPTH_TEST);

    GLuint vertexShader = load_and_compile_shader("src/shaders/vertex.glsl", GL_VERTEX_SHADER);
    GLuint fragmentShader = load_and_compile_shader("src/shaders/fragment.glsl", GL_FRAGMENT_SHADER);
    GLuint shaderProgram = attach_shaders(2, vertexShader, fragmentShader);
    glUseProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Generate base sphere geometry (radius 1.0, scaled later)
    float* sphereVertices;
    int vertexCount;
    generateSphere(1.0f, 36, 18, &sphereVertices, &vertexCount);

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 6 * sizeof(float), sphereVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Get Uniform Locations
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLint lightColLoc = glGetUniformLocation(shaderProgram, "lightColor");

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        updatePhysics();

        glClearColor(0.02f, 0.02f, 0.05f, 1.0f); // Deep space background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set up Camera / View matrix
        mat4 view;
        vec3 cameraPos;
        
        // Convert spherical to Cartesian coordinates
        cameraPos[0] = cameraRadius * cos(glm_rad(cameraYaw)) * cos(glm_rad(cameraPitch));
        cameraPos[1] = cameraRadius * sin(glm_rad(cameraPitch));
        cameraPos[2] = cameraRadius * sin(glm_rad(cameraYaw)) * cos(glm_rad(cameraPitch));

        vec3 cameraTarget = {0.0f, 0.0f, 0.0f}; // Always look at the center
        vec3 cameraUp = {0.0f, 1.0f, 0.0f};
        
        glm_lookat(cameraPos, cameraTarget, cameraUp, view);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, (float*)view);

        // Set up Projection matrix
        mat4 projection;
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glm_perspective(glm_rad(45.0f), (float)width / (float)height, 0.1f, 1000.0f, projection);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, (float*)projection);

        // Set Light Source (Coming from the central star)
        glUniform3f(lightPosLoc, b1.x, b1.y, b1.z);
        glUniform3f(lightColLoc, 1.0f, 1.0f, 1.0f);

        glBindVertexArray(VAO);

        // Render Body 1 (Star)
        mat4 model1;
        glm_mat4_identity(model1);
        glm_translate(model1, (vec3){b1.x, b1.y, b1.z});
        glm_scale(model1, (vec3){b1.radius, b1.radius, b1.radius});
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float*)model1);
        glUniform3f(colorLoc, b1.r, b1.g, b1.b);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        // Render Body 2 (Planet)
        mat4 model2;
        glm_mat4_identity(model2);
        glm_translate(model2, (vec3){b2.x, b2.y, b2.z});
        glm_scale(model2, (vec3){b2.radius, b2.radius, b2.radius});
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float*)model2);
        glUniform3f(colorLoc, b2.r, b2.g, b2.b);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    free(sphereVertices);
    glfwTerminate();
    return 0;
}
