#include <WinMain.h>

int main(int argc, char** argv)
{
        GLFWwindow*       window    = make_fullscreen_window();
        const char*       prog_name = "prog1";

        make_shad_prog_n_res_man(argv[0], prog_name);
        GLuint shad_prog = get_shader_prog_id(prog_name);

        WinMain(window, shad_prog);

        destroy_resource_manager();
        glfwTerminate();

        return 0;
}
