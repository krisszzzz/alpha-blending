#include "WinMain.h"

#include <WinMain.h>
#include <emmintrin.h>
#include <time.h>
#include <immintrin.h>
#include <cmath>

#define PIXEL_COORD_COUNT    2
#define WINDOW_WIDTH         1920
#define WINDOW_HEIGHT        1080
#define PIXELS_COUNT         WINDOW_WIDTH * WINDOW_HEIGHT
#define POINTS_COUNT         PIXELS_COUNT * PIXEL_COORD_COUNT
#define COLORS_COUNT         PIXELS_COUNT

void glfw_key_callback(GLFWwindow* p_window, int key, int scancode, int action, int mode)
{
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                glfwSetWindowShouldClose(p_window, GLFW_TRUE);
        }
}

GLFWwindow* make_fullscreen_window()
{
    /* Initialize the library */
        if (!glfwInit()) {
                ErrorPrint("GLFW library initializitation error ocurred\n");
                return nullptr;
        }

    // Required minimum 4.6 OpenGL
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

   /* Create a windowed mode window and its OpenGL context */
        GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH,
                                              WINDOW_HEIGHT,
                                              "Hello World",
                                              glfwGetPrimaryMonitor(),
                                              NULL);

        if (!window) {
                ErrorPrint("Window creating error\n");
                glfwTerminate();
                return nullptr;
        }

        glfwSetKeyCallback(window, glfw_key_callback);
   /* Make the window's context current */
        glfwMakeContextCurrent(window);

        if (!gladLoadGL()) {
                ErrorPrint("Fatal error: GLAD cannot be loaded\n");
                return nullptr;
        }

        printf("OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);
        glClearColor(0, 0, 0, 1);

        return window;

}

int make_shad_prog_n_res_man(const char *const execution_path, const char *const shader_prog_name)
{
        resource_manager res_man = {};
        init_resource_manager(&res_man, execution_path);
        bind_resource_manager(res_man);

        create_shader_prog(shader_prog_name, "res/Shaders/vertex_shader.glsl", "res/Shaders/fragment_shader.glsl");
        resource_manager_shader_log();

        return 0;

}

static void generate_buffers(GLfloat   points[POINTS_COUNT], GLfloat colors[COLORS_COUNT],
                             GLuint*   p_points_vbo,         GLuint* p_colors_vbo,
                             GLuint*   p_vao)
{
        int buffer_count  = 1;
        GLuint points_vbo = 0;
        GLuint colors_vbo = 0;
        GLuint vao        = 0;

        glGenBuffers(buffer_count, &points_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
        glBufferData(GL_ARRAY_BUFFER, POINTS_COUNT * sizeof(GLfloat), points, GL_DYNAMIC_DRAW);

        glGenBuffers(buffer_count, &colors_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, colors_vbo);
        glBufferData(GL_ARRAY_BUFFER, COLORS_COUNT * sizeof(GLfloat), colors, GL_DYNAMIC_DRAW);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
        glVertexAttribPointer(0, PIXEL_COORD_COUNT, GL_FLOAT, GL_FALSE, 0, nullptr);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colors_vbo);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, nullptr);


        *p_points_vbo = points_vbo;
        *p_vao       = vao;
        *p_colors_vbo = colors_vbo;

}

void set_position(GLfloat* points, int x, int y)
{
        GLfloat x_pos = ((float)x - WINDOW_WIDTH  / 2.0f) / (WINDOW_WIDTH / 2.0f);
        GLfloat y_pos = ((float)-y + WINDOW_HEIGHT / 2.0f) / (WINDOW_HEIGHT / 2.0f);

        points[2*x +  2 * y * WINDOW_WIDTH]     = x_pos;
        points[2*x +  2 * y * WINDOW_WIDTH + 1] = y_pos;
}

void init_points(float* points)
{
        for (int x = 0; x < WINDOW_WIDTH; ++x) {
                for (int y = 0; y < WINDOW_HEIGHT; ++y) {
                        set_position(points, x, y);
                }
        }
}

void WinMain(GLFWwindow* window, GLuint shader_program)
{
        int vertex_indx_start = 0;
        int vertex_indx_end   = PIXELS_COUNT;
        int vertex_position   = 0;
        int color_position    = 1;

        GLfloat* points = (GLfloat*)calloc(POINTS_COUNT, sizeof(GLfloat));
        GLfloat* colors = (GLfloat*)calloc(COLORS_COUNT, sizeof(GLfloat));

        init_points(points);

        GLuint points_vbo  = 0;
        GLuint colors_vbo  = 0;
        GLuint vao         = 0;

        generate_buffers(points, colors, &points_vbo, &colors_vbo, &vao);
        clock_t stop    = 0,
                start   = 0;

       while (!glfwWindowShouldClose(window)) {
                /* Render here */

                start  = clock();
                glClear(GL_COLOR_BUFFER_BIT);

                generate_color_mandelbrot(colors, points);

                glBindBuffer(GL_ARRAY_BUFFER, colors_vbo);
                glBufferData(GL_ARRAY_BUFFER, COLORS_COUNT * sizeof(GLfloat), colors, GL_DYNAMIC_DRAW);

                glVertexAttribPointer(color_position, 1,  GL_FLOAT,
                                      GL_FALSE,       0,  nullptr);

                glUseProgram(shader_program);  // Use shader program that was
                glBindVertexArray(vao);        // generated bu glfw_make_triangle
                                               // Bind current VAO
                glDrawArrays(GL_POINTS, vertex_indx_start,
                             vertex_indx_end);           // Draw current VAO
        /* Swap front and back buffers */
                glfwSwapBuffers(window);
                /* Poll for and process events */
                glfwPollEvents();

                stop = clock();

                printf("FPS: %lf\n", 1000000.0 / (stop - start));

//                first_time = second_time;

        }

       free(points);
       free(colors);
}
