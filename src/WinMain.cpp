#include <WinMain.h>
#include <emmintrin.h>
#include <time.h>
#include <immintrin.h>
#include <cmath>
#include <stdalign.h>

#define PIXEL_COORD_COUNT    2
#define WINDOW_WIDTH         800 
#define WINDOW_HEIGHT        600 

#define PIXELS_COUNT         WINDOW_WIDTH * WINDOW_HEIGHT
#define POINTS_COUNT         PIXELS_COUNT * PIXEL_COORD_COUNT
#define COLOR_MODEL          4
#define COLORS_COUNT         PIXELS_COUNT * COLOR_MODEL 

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


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
                                          NULL,
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
    glVertexAttribPointer(1, COLOR_MODEL, GL_FLOAT, GL_FALSE, 0, nullptr);

    *p_points_vbo = points_vbo;
    *p_vao        = vao;
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

void alpha_blending(GLfloat*       colors         ,  unsigned char* rgba_background,
                    unsigned char* rgba_foreground,  const int      fg_left_corner_x,
                    const int      fg_left_corner_y, const int      fg_width,
                    const int      fg_height)
{
    for (int y = 0; y < fg_height * COLOR_MODEL; y += 4) {
        for (int x = 0; x < fg_width * COLOR_MODEL; x += 4) {
            int fg_pos   = x + y * fg_width; 
            int bg_pos   = x + COLOR_MODEL * fg_left_corner_x + (y + COLOR_MODEL * fg_left_corner_y) * WINDOW_WIDTH;
            float alpha  = rgba_foreground[fg_pos + 3] / 255.0f;
            colors[bg_pos]                   = (rgba_foreground[fg_pos] * alpha +
                                                rgba_background[bg_pos] * (1.0f - alpha)) / 255.0f;
            colors[bg_pos + 1]               = (rgba_foreground[fg_pos + 1] * alpha +
                                                rgba_background[bg_pos + 1] * (1.0f - alpha)) / 255.0f;
            colors[bg_pos + 2]               = (rgba_foreground[fg_pos + 2] * alpha +
                                                rgba_background[bg_pos + 2] * (1.0f - alpha)) / 255.0f;
        }
    }
}

void alpha_blending_avx2(GLfloat*       colors         ,  unsigned char* rgba_background,
                         unsigned char* rgba_foreground,  const int      fg_left_corner_x,
                         const int      fg_left_corner_y, const int      fg_width,
                         const int      fg_height)
{
    for (int y = 0; y < fg_height * COLOR_MODEL; y += 4) {
        for (int x = 0; x < fg_width * COLOR_MODEL; x += 4) {
            
        }
    }
}

void load_background(GLfloat* colors, unsigned char* rgba_source)
{
    for (int y = 0; y < COLOR_MODEL * WINDOW_HEIGHT; y += 4) {
        int curr_y = y * WINDOW_WIDTH;
        for (int x = 0; x < COLOR_MODEL * WINDOW_WIDTH; x += 8) {
            
            //            __m256 _8_full_rgba_model  = _mm256_load_si256((__m256*)&rgba_source[curr_y]);
            __m256 _2_full_rgba_model  = _mm256_set_ps((float)rgba_source[curr_y + (x + 7)],
                                                       (float)rgba_source[curr_y + (x + 6)],
                                                       (float)rgba_source[curr_y + (x + 5)],
                                                       (float)rgba_source[curr_y + (x + 4)],
                                                       (float)rgba_source[curr_y + (x + 3)],
                                                       (float)rgba_source[curr_y + (x + 2)],
                                                       (float)rgba_source[curr_y + (x + 1)],
                                                       (float)rgba_source[curr_y + (x + 0)]);
            
            __m256 _255f               = _mm256_set1_ps(1.0f / 255.0f);
            __m256 normalized_col      = _mm256_mul_ps(_255f, _2_full_rgba_model);
            
            _mm256_storeu_ps(&colors[x + y * WINDOW_WIDTH], normalized_col);
        }
    } 
}

void WinMain(GLFWwindow* window, GLuint shader_program)
{
    int vertex_indx_start = 0;
    int vertex_indx_end   = PIXELS_COUNT;
    int color_position    = 1;
    
    alignas(32) GLfloat* points = (GLfloat*)calloc(POINTS_COUNT, sizeof(GLfloat)); 
    alignas(32) GLfloat* colors = (GLfloat*)calloc(COLORS_COUNT, sizeof(GLfloat));

    int back_width  = 0;
    int back_height = 0;
    int back_comp   = 0;

    int fg_width    = 0;
    int fg_height   = 0;
    int fg_comp     = 0;
    
    alignas(32) unsigned char* back_image = stbi_load("res/Table.bmp",     &back_width, &back_height, &back_comp, 4);
    alignas(32) unsigned char* for_image  = stbi_load("res/AskhatCat.bmp", &fg_width,   &fg_height,   &fg_comp,   4);
    
    init_points(points);
    load_background(colors, back_image); // load back_image to normalized form into GLfloat* colors
                                     // and use it further to load textures
                                     //my cringe version of manual textures loading

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

        load_background(colors, back_image); // load back_image to normalized form into GLfloat* colors
        alpha_blending(colors, back_image, for_image,  250, 200, fg_width, fg_height);
            
        glBindBuffer(GL_ARRAY_BUFFER, colors_vbo);
        glBufferData(GL_ARRAY_BUFFER, COLORS_COUNT * sizeof(GLfloat), colors, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(color_position, COLOR_MODEL,  GL_FLOAT,
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
    
