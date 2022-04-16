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

void alpha_blending(float*     rgba_background,  float*    rgba_foreground,
                    const int  fg_left_corner_x, const int fg_left_corner_y,
                    const int  fg_width,         const int fg_height)
{
    for (int y = 0; y < fg_height * COLOR_MODEL; y += 4) {
        for (int x = 0; x < fg_width * COLOR_MODEL; x += 4) {
            int fg_pos   = x + y * fg_width; 
            int bg_pos   = x + COLOR_MODEL * fg_left_corner_x + (y + COLOR_MODEL * fg_left_corner_y) * WINDOW_WIDTH;
            float alpha  = rgba_foreground[fg_pos + 3];
            rgba_background[bg_pos]       = (rgba_foreground[fg_pos] * alpha +
                                             rgba_background[bg_pos] * (1.0f - alpha));
            rgba_background[bg_pos + 1]   = (rgba_foreground[fg_pos + 1] * alpha +
                                             rgba_background[bg_pos + 1] * (1.0f - alpha));
            rgba_background[bg_pos + 2]   = (rgba_foreground[fg_pos + 2] * alpha +
                                             rgba_background[bg_pos + 2] * (1.0f - alpha));
        }
    }
}

inline void alpha_blending_avx2(float*    background,       float*    foreground,
                                const int fg_left_corner_x, const int fg_left_corner_y,
                                const int fg_width,         const int fg_height)  __attribute__((hot, nothrow));

inline void alpha_blending_avx2(float*    background,       float*    foreground,
                                const int fg_left_corner_x, const int fg_left_corner_y,
                                const int fg_width,         const int fg_height)
{
    for (int y = 0; y < fg_height * COLOR_MODEL; y += 4) {
        for (int x = 0; x < fg_width * COLOR_MODEL; x += 8) {

            int fg_pos   = x + y * fg_width; 
            int bg_pos   = x + COLOR_MODEL * fg_left_corner_x + (y + COLOR_MODEL * fg_left_corner_y) * WINDOW_WIDTH;
            // fg:
            //  7  6  5  4    3  2  1  0
            // [r2 g2 b2 a2 | r1 g1 b1 a1]
            
            
            __m256 back      = _mm256_loadu_ps(&background[bg_pos]);  // Latency = 3
            __m256 front     = _mm256_loadu_ps(&foreground[fg_pos]);  // Latency = 3

            __m256  alpha_h_2 = _mm256_movehdup_ps(_mm256_blend_ps(_mm256_setzero_ps(), front, 0x88));
            __m256d alpha_l_2 = _mm256_castsi256_pd(_mm256_bsrli_epi128(_mm256_castps_si256(alpha_h_2), 8));
            __m256  alpha     = _mm256_castpd_ps(_mm256_movedup_pd(alpha_l_2));
           
           __m256 blended = _mm256_add_ps(_mm256_sub_ps(_mm256_mul_ps(front, alpha), _mm256_mul_ps(back, alpha)), back);

            _mm256_storeu_ps(&background[bg_pos], blended);
            
            // Don't be disordered, the little-endian add some expressions that hard to explain. Such as integer mask 0x88
            //                                                                                               0x88 = 10001000b 
            // 1) _m256_blend_ps(_mm256_setzero_ps(), front, 10001000b) -> ret_val1
            // fg:
            //  7  6  5  4    3  2  1  0
            // [r2 g2 b2 a2 | r1 g1 b1 a1]
            //           |
            // ret_val1: V 
            //  7  6  5  4    3  2  1   0
            // [0  0  0  a2 | 0  0  0  a1]
                   
            // 2) _mm256_movehdup_ps(_mm256_blend_ps(_mm256_setzero_ps(), front, 00010001b)) -> alpha_h_2

            // ret_val1:       
            //              7  6  5  4    3  2  1   0
            //             [0  0  0  a2 | 0  0  0  a1]
            //                       /             /
            //                      /             /
            //                     /             /
            //                    ,             ,
            //                    |             |
            //                    |             |
            //                    |             |
            // alpha_h_2:         V             V
            //             7  6   5   4   3  2  1   0
            //             [0  0  a2  a2 | 0  0 a1  a1] 
            //
            //  3) _mm256_castsi256_pd(_mm256_bsrli_epi128(_mm256_castps_si256(alpha_h_2), 8)) -> alpha_l_2
            // alpha_h_2:
            //              7  6   5   4   3  2  1   0
            //             [0  0  a2  a2 | 0  0  a1  a1] 
            //                    /   /         /    /
            //                   /   /         /    /
            //              +---,   /      +--/+---/
            //              |  +---,       |   |
            //              |  |           |   |
            //              |  |           |   |
            //              |  |           |   |
            // alpha_l_2:   |  |           |   |
            //              V  V           V   V
            //              7  6   5   4   3   2  1   0
            //             [a2  a2  0  0 | a1  a1 0   0] 
            //
            //  4) _mm256_castpd_ps(_mm256_movedup_pd(alpha_l_2)) -> alpha
            // alpha_l_2:
            //   
            //              7   6    5   4   3   2   1  0
            //             [a2  a2   0   0 | a1  a1  0  0] 
            //              \   \            \   \.
            //               \   \+-----+     \   \.
            //                \.        |      \.  ,-----+
            //                 ,----+   |       ,----+   |
            //                      |   |            |   |
            // alpha:               V   V            V   V
            //             [a2  a2  a2  a2 | a1  a1  a1  a1]
        }         
    }
}

void WinMain(GLFWwindow* window, GLuint shader_program)
{
    int vertex_indx_start = 0;
    int vertex_indx_end   = PIXELS_COUNT;
    int color_position    = 1;
    
    GLfloat* points = (GLfloat*)calloc(POINTS_COUNT, sizeof(GLfloat)); 
    GLfloat* colors = (GLfloat*)calloc(COLORS_COUNT, sizeof(GLfloat));

    int back_width  = 0;
    int back_height = 0;
    int back_comp   = 0;

    int fg_width    = 0;
    int fg_height   = 0;
    int fg_comp     = 0;
    
    float* back_image = stbi_loadf("res/Table.bmp",     &back_width, &back_height, &back_comp, 4);
    float* for_image  = stbi_loadf("res/AskhatCat.bmp", &fg_width,   &fg_height,   &fg_comp,   4);
    
    init_points(points);
    //    load_background(colors, back_image); // load back_image to normalized form into GLfloat* colors
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

        glClear(GL_COLOR_BUFFER_BIT);

        start  = clock();
        for (int iter_count = 0; iter_count < 1000000; ++iter_count) {
            alpha_blending(back_image, for_image,  250, 200, fg_width, fg_height);
        }
        // alpha_blending test
            
        stop = clock();
        double no_avx_time = (stop - start) / 1000000.0;
        
        start  = clock();
        for (int iter_count = 0; iter_count < 1000000; ++iter_count) {
            alpha_blending_avx2(back_image, for_image,  250, 200, fg_width, fg_height);
        }
        // alpha_blending with avx2 instructions test
            
        stop = clock();
        double avx_time = (stop - start) / 1000000.0;
        printf("with AVX2: %lf; without AVX2: %lf; %lf times is faster\n", avx_time, no_avx_time, no_avx_time / avx_time);
        // compare and show results
        
        glBindBuffer(GL_ARRAY_BUFFER, colors_vbo);
        glBufferData(GL_ARRAY_BUFFER, COLORS_COUNT * sizeof(GLfloat), back_image, GL_DYNAMIC_DRAW);

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



        //                first_time = second_time;

    }

    free(points);
    free(colors);
}
    
