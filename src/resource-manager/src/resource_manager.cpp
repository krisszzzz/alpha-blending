#include <resource_manager.h>
#include <log.h>
#include <string.h>
#include <malloc.h>

resource_manager binded_manager  = {};
shader_program*  NOT_FOUNDED     = (shader_program*)997;


#define MIN_CHUNK_SIZE      2048
#define MIN_PROGRAM_COUNT   24

#define set_shad_status(value)                                                                            \
        prog->shaders[iter_count].status = save_bit(prog->shaders[iter_count].status, \
                                                    value);

#define set_shad_prog_status(check_status) \
        prog->status = save_bit(prog->status, check_status)


#define SHADER(iter_count) prog->shaders[iter_count]


// ----------------------------------------------------------------------------------
// Resource manager

static int save_shader_path(shader* shader_to_save_path, const char* source)
{
        if (source == nullptr) {
                ErrorPrint(RED "ERROR: " END "nullptr source\n");
                return RME_NULLPTR;
        }

        int source_len = strlen(source);

        if (source_len + binded_manager.shader_names_offset >= binded_manager.shader_names_size) {
                binded_manager.shader_names_size *= 2; // Double the allocated memory
                binded_manager.shader_names_chunk = (char*)realloc(binded_manager.shader_names_chunk,
                                                                   binded_manager.shader_names_size * sizeof(char));

                if(binded_manager.shader_names_chunk == nullptr) {
                        ErrorPrint(RED "ERROR: " END "Memory reallocation error\n");
                        return RME_INCCORECT_MANAGER;
                }
        }

        shader_to_save_path->shader_path =
                &binded_manager.shader_names_chunk[binded_manager.shader_names_offset];

        strncpy(shader_to_save_path->shader_path,
                source,
                source_len);


        int gap_size = 1;

        binded_manager.shader_names_offset += source_len; // Skip one
        binded_manager.shader_names_chunk[binded_manager.shader_names_offset] = 0;
        binded_manager.shader_names_offset += gap_size; // Skip one


        return 0;

}

static int find_exe_file_name_len(const char* exec_path) {

        int len = 0;
        for(int iter_count = strlen(exec_path); iter_count >= 0 && exec_path[iter_count] != '/'; --iter_count) {
                len++;
        }
        int backslash_position = len - 1;

        return backslash_position;
}

int init_resource_manager(resource_manager* res_manager, const char* exec_path)
{
        if (res_manager == nullptr) {
                ErrorPrint(RED "ERROR: " END "resource_manager initialization error. Nullptr resource_manager\n");
                return NULLPTR;
        }

        if (exec_path == nullptr) {
                ErrorPrint(RED "ERROR: " END "Exec_path is nullptr\n");
                return NULLPTR;
        }
        int exec_path_len        = strlen(exec_path) - find_exe_file_name_len(exec_path);

        char* shader_names_chunk = (char*)calloc(MIN_CHUNK_SIZE, sizeof(char));
        char* execute_path       = (char*)calloc(exec_path_len,
                                                 sizeof(char));

        if(shader_names_chunk == nullptr || execute_path == nullptr) {
                ErrorPrint(RED "ERROR: " END "Memory allocation error\n");
                return RME_MALLOC_ERROR;
        }

        strncpy(execute_path, exec_path, exec_path_len);

        res_manager->shader_names_chunk  = shader_names_chunk;
        res_manager->execute_path        = execute_path;
        res_manager->is_initialized      = 1;
        res_manager->shader_names_offset = 0;
        res_manager->shader_names_size   = MIN_CHUNK_SIZE;

        return 0;

}

int bind_resource_manager(resource_manager res_manager)
{
        res_manager.is_binded = 1;
        binded_manager        = res_manager;
        return 0;

}

shader_program* find_shader_prog(const char* prog_name)
{
        if (!binded_manager.is_binded) {
                ErrorPrint(RED "ERROR: " END "No resource manager is binded\n");
                return nullptr;
        }

        if (binded_manager.programs == nullptr) {
                ErrorPrint(RED "ERROR:" END "No program was located. The programs pointer is nullptr\n");
                return nullptr;
        }

        hash_t hash_to_find = MurmurHash(prog_name, strlen(prog_name));

        for (int iter_count = 0; iter_count < binded_manager.shader_program_count; ++iter_count) {
                if (binded_manager.programs[iter_count].shader_prog_hash == hash_to_find) {
                        return &binded_manager.programs[iter_count];
                }
        }

        return NOT_FOUNDED;
}


GLuint get_shader_prog_id(const char* prog_name)
{
        shader_program* program = find_shader_prog(prog_name);

        return program == NOT_FOUNDED ? 0 : program->shader_prog_id;
}


//++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+--+-
// Working with files

char* file_to_buffer(FILE* source, int* buffer_size)
{
        if (source == nullptr) {
                ErrorPrint(RED "ERROR: " END " nullptr FILE pointer in file_to_buffer\n");
                return nullptr;
        }

        fseek(source, 0, SEEK_END);
	int file_size = ftell(source);
	fseek(source, 0, SEEK_SET);

	char* buffer = (char*)calloc(file_size + 1, sizeof(char));

        if (buffer == nullptr) {

                ErrorPrint(RED "ERROR:" END " memory allocation error\n");
                return nullptr;
        }

        if (buffer_size) {
                *buffer_size = file_size;
        }

        fread(buffer, sizeof(char), file_size, source);
	return buffer;
}

char* load_file_source(const char *const src_file_path)
{
        if (!src_file_path) {
                ErrorPrint("src_file_path is nullptr.\n");
                return nullptr;
        }

        FILE *source_file = fopen(src_file_path, "r");

        if (!source_file) {
                ErrorPrint(RED "ERROR: " END " cannot open file. File name : %s\n", src_file_path);
                return nullptr;
        }

        char* source_buffer = file_to_buffer(source_file, nullptr);
        fclose(source_file);

        if (!source_buffer) {
                ErrorPrint(RED "ERROR: " END " memory allocation error\n");
                return nullptr;
        }

        return source_buffer;
}

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
// Checkers, validators, dump etc.

static inline int check_shader_status(long long shader_status, int status)
{
        return (shader_status >> status) & 1; // Get status bit
                                                          //
}

static inline long long save_bit(long long value, int bit_position)
{
        return value | (1 << bit_position);
}

static inline GLenum ret_shader_type_by_iter_count(int iter_count)
{
        switch(iter_count) {
                case 0:
                        return GL_VERTEX_SHADER;
                case 1:
                        return GL_FRAGMENT_SHADER;
                case 2:
                        return GL_GEOMETRY_SHADER;
                case 3:
                        return GL_TESS_CONTROL_SHADER;
                case 4:
                        return GL_TESS_EVALUATION_SHADER;
                case 5:
                        return GL_COMPUTE_SHADER;
                default:
                        return -1;
        }
}

static inline const char* shader_type_str_by_iter_count(GLenum shader_type)
{
        switch(shader_type) {
                case 0:
                        return "GL_VERTEX_SHADER";
                case 1:
                        return "GL_FRAGMENT_SHADER";
                case 2:
                        return "GL_GEOMETRY_SHADER";
                case 3:
                        return "GL_TESS_CONTROL_SHADER";
                case 4:
                        return "GL_TESS_EVALUATION_SHADER";
                case 5:
                        return "GL_COMPUTE_SHADER";
                default:
                        return nullptr;
        }
}

static inline void print_shader_errors(shader* shaders)
{
        if (check_shader_status(shaders->status, SHADERS_COMPILATION_FILE_LOAD_ERROR)) {
                printf(RED "FILE_LOAD_ERROR " END);
        }

        if (check_shader_status(shaders->status, SHADERS_COMPILATION_SYNTAX_ERROR)) {
                printf(RED "SYNTAX_ERROR " END);
        }

        if (check_shader_status(shaders->status, SHADERS_LINKING_COMPILE_ERROR)) {
                printf(RED "LINKING_ON_COMPILE_ERROR " END);
        }

        if (check_shader_status(shaders->status, SHADERS_LINKING_GL_ATTACH_ERROR)) {
                printf(RED "GL_ATTACH_ERROR " END);
        }

        printf("\n");

}


static int shader_log(shader* shaders, int iter_count)
{
        if (shaders == nullptr) {
                ErrorPrint(RED "ERROR: " END "nullptr shader was unexpected\n");
                return NULLPTR;
        }

        printf(BLUE   "__________________________________"
                      "__________________________________\n" END);


                printf(BLUE "%-25s " END, shader_type_str_by_iter_count(iter_count));

                if (check_shader_status(shaders->status, SHADERS_COMPILED) &&
                    check_shader_status(shaders->status, SHADERS_LINKED)) {
                        printf(GREEN);
                } else {
                        printf(RED "*");
                }


                printf("%s\n", shaders->shader_path);

                printf(BLUE "shader info:\n"
                            "shader_id = %u\n"
                            "shader_hash = %llu\n" END,
                            shaders->shader_id,
                            shaders->shader_hash
                       );

                print_shader_errors(shaders);

        printf(BLUE   "__________________________________"
                      "__________________________________\n\n" END);

                return 0;
}


static int shader_program_log(shader_program* prog)
{
        if (prog == nullptr) {
                ErrorPrint(RED "nullptr prog\n" END);
                return NULLPTR;
        }
        printf(CYAN "__________________________________"
                    "__________________________________\n"
                    "SHADER_PROGRAM LOG\n"
                    "shader_program log:\n"
                    "shader_prog_id    = %u\n"
                    "shader_prog_hash  = %llu\n" END,
                    prog->shader_prog_id,
                    prog->shader_prog_hash);

        for(int iter_count = 0; iter_count < MAX_SHADER_TYPES; ++iter_count) {
                if (SHADER(iter_count).status == SHADERS_NO_STATUS) {
                        printf(BLUE "%-25s " RED "UNUSED\n" END, shader_type_str_by_iter_count(iter_count));
                        continue;
                }
                shader_log(&SHADER(iter_count), iter_count);

        }

        printf(CYAN "__________________________________"
                    "__________________________________\n\n" END);
        return 0;

}


int resource_manager_shader_log()
{

        if (!binded_manager.is_binded || !binded_manager.is_initialized) {
                ErrorPrint("Resource manager unititilized or unbinded\n");
                return RME_INCCORECT_MANAGER;
        }

        printf(RED "__________________________________"
                   "__________________________________\n");

        printf(RED "RESOURCE MANAGER LOG:\n"
                   "Resource manager info: \n"
                   "execute_path = " GREEN " %s\n" RED
                   "shader_names_offset = %d\n"
                   "shader_names_size = %d\n"
                   "shader_program_count = %d\n"
                   "shader_program_size = %d\n" END,
               binded_manager.execute_path,
               binded_manager.shader_names_offset,
               binded_manager.shader_names_size,
               binded_manager.shader_program_count,
               binded_manager.shader_program_size);


        for(int iter_count = 0; iter_count < binded_manager.shader_program_count; ++iter_count) {
                shader_program_log(&binded_manager.programs[iter_count]);
        }

        printf(RED "__________________________________"
                   "__________________________________\n" END);
        return 0;

}


static int attach_shader(shader_program* prog, int iter_count)
{
        if (prog == nullptr) {
                ErrorPrint(FATAL_RED "Error: " END
                           "shader program is nullptr\n");
                return NULLPTR;
        }


        if (check_shader_status(SHADER(iter_count).status,
                                SHADERS_COMPILATION_FILE_LOAD_ERROR) ||
            check_shader_status(SHADER(iter_count).status,
                                SHADERS_COMPILATION_SYNTAX_ERROR)) {
                set_shad_status(SHADERS_LINKING_COMPILE_ERROR);
        }


       glAttachShader(prog->shader_prog_id,
                       SHADER(iter_count).shader_id);
        GLenum check_error = glGetError();

        if(check_error != GL_NO_ERROR) {
                set_shad_status(SHADERS_LINKING_GL_ATTACH_ERROR);
                return ERROR_RET;
        }

        set_shad_status(SHADERS_LINKED);

        return 0;
}

static int link_prog(shader_program* prog)
{
        glLinkProgram(prog->shader_prog_id);
        glValidateProgram(prog->shader_prog_id);

        GLint get_programiv_result = 0;
        glGetProgramiv(prog->shader_prog_id, GL_VALIDATE_STATUS, &get_programiv_result);

        if(get_programiv_result == GL_FALSE) {

                set_shad_prog_status(SHADER_PROG_GL_VALIDATE_ERROR);

                printf(RED "ERROR:" END
                       "Program validation error\n");

                glGetProgramiv(prog->shader_prog_id, GL_ATTACHED_SHADERS, &get_programiv_result);

                printf("Attached shaders count: %d\n", get_programiv_result);
                printf("Program log:\n");
                GLsizei log_length = 0;

                glGetProgramiv(prog->shader_prog_id, GL_INFO_LOG_LENGTH, &log_length);

                if(log_length != 0) {
                        char* log_buffer = (char*)calloc(log_length, sizeof(char));
                        glGetProgramInfoLog(prog->shader_prog_id, log_length, &log_length, log_buffer);
                        printf(log_buffer);
                        free(log_buffer);

                } else {
                        printf("No information provided\n");
                }

        }

        return 0;
}

// ++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+--+-
// Compilation stage
static int shader_compile_status(shader_program* prog, int iter_count)
{
        int result = 0;

        glGetShaderiv(SHADER(iter_count).shader_id, GL_COMPILE_STATUS, &result);

        if(result == GL_FALSE) {  // Compilation syntax error

                set_shad_status(SHADERS_COMPILATION_SYNTAX_ERROR);
                printf(RED "Shader compilation error\n" END);

                int log_length = 0;

                glGetShaderiv(SHADER(iter_count).shader_id,
                              GL_INFO_LOG_LENGTH,
                              &log_length
                              );

                char* message = (char*) calloc(log_length, sizeof(char));

                if (message == nullptr) {
                        ErrorPrint(RED "ERROR:" END "memory allocation error, cannot make log about"
                                   "inccorect shader compilation\n");
                        set_shad_prog_status(SHADER_PROG_LOG_MALLOC_ERROR);
                        return NULLPTR;
                }

                glGetShaderInfoLog(SHADER(iter_count).shader_id,
                                   log_length,
                                   &log_length,
                                   message
                                   );

                printf("Failed to compile shader file. Shader file path " RED " %s\n" END,
                       SHADER(iter_count).shader_path);
                printf("Compilation log: \n");
                printf("%s\n\n\n", message);

                free(message);

                return -1;
        }
        return 0;
}


#define set_res_status(status)                                          \
        do {                                                            \
        binded_manager.status = save_bit(binded_manager.status, status) \
        } while(0)

int create_shader_prog(const char* const shader_prog_name, const char* const vert_s,
                       const char* const frag_s,           const char* const geom_s,
                       const char* const tess_ctl_s,       const char* const tess_eval_s,
                       const char* const comp_s)
{
        // TODO::::::: REWORK RESOURCE_MANAGER_HANDLER_ERROR
        if (!binded_manager.is_initialized || !binded_manager.is_binded) {
                ErrorPrint(RED "Resource manager error: " END
                           "Probably you forgot to bind or/and initialize "
                           "resource manager\n");

                return ERROR_RET;
        }

        if (shader_prog_name == nullptr) {
                ErrorPrint(RED "Error: " END
                           "Nullptr shader ");
        }

        if (binded_manager.shader_program_size == 0) {
                binded_manager.programs = (shader_program*)calloc(MIN_PROGRAM_COUNT,
                                                                  sizeof(shader_program));
                binded_manager.shader_program_size = MIN_PROGRAM_COUNT;
                if (binded_manager.programs == nullptr) {
                        ErrorPrint(RED "Memory allocation for programs was errored" END);
                        // Set resource manager status
                        return  RME_MALLOC_ERROR;
                }
        }

        if (binded_manager.shader_program_count >= binded_manager.shader_program_size - 1) {
                binded_manager.shader_program_size *= 2;
                binded_manager.programs = (shader_program*)realloc(binded_manager.programs,
                                                                   sizeof(shader_program) *
                                                                   binded_manager.shader_program_size);

                if (binded_manager.programs == nullptr) {
                        ErrorPrint(RED "Memory allocation for programs was errored" END);
                        // Set resource manager status
                        return  RME_MALLOC_ERROR;
                }
        }

        const char* const shaders_path[MAX_SHADER_TYPES] =  {vert_s,     frag_s,      geom_s,
                                                             tess_ctl_s, tess_eval_s, comp_s};

        shader_program* prog   = &binded_manager.programs[binded_manager.shader_program_count++];
        prog->shader_prog_hash = MurmurHash(shader_prog_name, strlen(shader_prog_name));
        prog->shader_prog_id   = glCreateProgram();

        const int k_string_buffer_size = 1;

        for (int iter_count = 0; iter_count < MAX_SHADER_TYPES; ++iter_count) {

                // Watch for empty prog
                if (shaders_path[iter_count] == nullptr) {
                        continue;
                }

                // Start loading file
                save_shader_path(&SHADER(iter_count), shaders_path[iter_count]);

                SHADER(iter_count).shader_hash  = MurmurHash(shaders_path[iter_count],
                                                             strlen(shaders_path[iter_count]));

                char* shader_source           = load_file_source(SHADER(iter_count).shader_path);
                SHADER(iter_count).shader_id  = glCreateShader(ret_shader_type_by_iter_count(iter_count));

                if (shader_source == nullptr) {
                        set_shad_status(SHADERS_COMPILATION_FILE_LOAD_ERROR);
                        continue;
                }

                glShaderSource(SHADER(iter_count).shader_id,
                               k_string_buffer_size,
                               &shader_source,
                               nullptr);

                // Compile shaders
                glCompileShader(SHADER(iter_count).shader_id);
                free(shader_source);

                if (shader_compile_status(prog, iter_count)) {
                   glDeleteShader(SHADER(iter_count).shader_id);
                   continue;
                }

                set_shad_status(SHADERS_COMPILED);  // Set status
                // Attaching shaders to program

                attach_shader(prog, iter_count);
                glDeleteShader(SHADER(iter_count).shader_id);
                // Delete unused shaders
        }

        link_prog(prog);

        return 0;

}

// ++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+--+-
// Linking


// ++-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+--+-
// Destructors

int destroy_programs()
{
        if (!binded_manager.is_binded || !binded_manager.is_initialized) {
                ErrorPrint("Resource manager unititilized or unbinded\n");
                return RME_INCCORECT_MANAGER;
        }

        if (binded_manager.programs == nullptr) {
                ErrorPrint("Destruction error. Programs are nullptr\n");
                return RME_NULLPTR;
        }

        for (int iter_count = 0; iter_count < binded_manager.shader_program_count; ++iter_count) {
                glDeleteProgram(binded_manager.programs[iter_count].shader_prog_id);
        }

        free(binded_manager.programs);

        return 0;
}

int destroy_resource_manager()
{
        if (!binded_manager.is_initialized || !binded_manager.is_binded) {
                printf(YELLOW "WARNING: " END "trying to destroy unitialized/unbinded resource "
                       "manager. Ignoring\n");
                return 0;
        }

        free(binded_manager.execute_path);
        free(binded_manager.shader_names_chunk);

        binded_manager.is_initialized       = 0;
        binded_manager.is_binded            = 0;
        binded_manager.shader_names_offset  = -1;
        binded_manager.shader_names_size    = -1;

        binded_manager.shader_program_size  = -1;
        binded_manager.shader_program_count = -1;

        return 0;
}


#undef SHADER
