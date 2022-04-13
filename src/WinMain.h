
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <log.h>
#include <resource_manager.h>


void glfw_key_callback(GLFWwindow* p_window, int key, int scancode, int action, int mode);

int make_ultra_shader(GLuint* p_shader_prog, GLuint* p_vao, GLuint* p_vbo, const char* path);

GLFWwindow* make_fullscreen_window();

int make_shad_prog_n_res_man(const char *const execution_path, const char *const shader_prog_name);

void WinMain(GLFWwindow* window, GLuint shader_program);
