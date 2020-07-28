#ifndef __SETL_SHELL_H__
#define __SETL_SHELL_H__  1

#ifdef __cplusplus
extern "C" {
#endif


#include <system.h>
#include <messages.h>

#undef SETL_SYSTEM_PROTO 
#undef SETL_SYSTEM_PROTO_VOID 
#undef SETL_SYSTEM 
#undef SETL_SYSTEM_VOID 

#ifdef TSAFE
#define SETL_SYSTEM_PROTO void* setl_instance,
#define SETL_SYSTEM_PROTO_VOID void* setl_instance
#define SETL_SYSTEM setl_instance,
#define SETL_SYSTEM_VOID setl_instance
#else
#define SETL_SYSTEM_PROTO 
#define SETL_SYSTEM_PROTO_VOID void
#define SETL_SYSTEM 
#define SETL_SYSTEM_VOID 
#endif


#ifdef SETL_IDE	

#undef EXTERNAL
#define EXTERNAL  

typedef void *(*PFN_Setl_Initialize)();
typedef void (*PFN_create_lib_file)(SETL_SYSTEM_PROTO const char *file);
typedef void  (*PFN_setl_set_io)(FILE *_stdin, FILE *_stdout, FILE *_stderr);
typedef int (**PFN_setl_printf) (const char *format, ...);
typedef int (**PFN_setl_fprintf) (FILE *file, const char *format, ...);
typedef int (**PFN_setl_fputs) (const char *string, FILE *file);
typedef void (**PFN_setl_exit) (int err_code);
typedef void (*PFN_compiler_init)(SETL_SYSTEM_PROTO_VOID);
typedef void (*PFN_runtime_cleanup)(SETL_SYSTEM_PROTO_VOID);
typedef void (*PFN_Setl_PrintVersion)(SETL_SYSTEM_PROTO_VOID);
typedef char **PFN_setl_lib_file;
typedef void (*PFN_Setl_SetCommandLine)(SETL_SYSTEM_PROTO int argc,int optind,char **argv);


typedef int (*PFN_compile_fragment)(SETL_SYSTEM_PROTO const char *program_source, int compile_flag);
typedef int (*PFN_set_compiler_options)(SETL_SYSTEM_PROTO const char *option,void *flag);
typedef int (*PFN_plugin_main)(SETL_SYSTEM_PROTO const char *fileName);
typedef int (*PFN_execute_go)(SETL_SYSTEM_PROTO int forever);
typedef void (*PFN_profiler_dump)(SETL_SYSTEM_PROTO_VOID);
typedef void (*PFN_setl_set_fname)(SETL_SYSTEM_PROTO const char *file);
typedef char *PFN_setl2_program_name;
typedef char *PFN_setl2_shlib_path;


#else

#undef EXTERNAL
#ifdef WIN32
#define EXTERNAL __declspec(dllimport) 
#else
#define EXTERNAL extern
#endif



EXTERNAL char setl2_program_name[64];
EXTERNAL char setl2_shlib_path[PATH_LENGTH+1];
EXTERNAL void (*setl_PostUrl) (char *msg, char *postdata);
EXTERNAL void (*setl_GetUrl) (char *msg);
EXTERNAL void (*setl_JavaScript) (char *msg);

EXTERNAL int (*setl_printf) (const char *format, ...);
EXTERNAL int (*setl_fprintf) (FILE *file, const char *format, ...);
EXTERNAL int (*setl_fputs) (const char *string, FILE *file);
EXTERNAL void (*setl_exit) (int err_code);
EXTERNAL void *(*setl_environment) (char *message);

EXTERNAL void *(*setl_malloc) (size_t size);
EXTERNAL void (*setl_free) (void *ptr);
EXTERNAL void (*setl_event_hook)();

EXTERNAL char *setl_lib_file;
EXTERNAL char *javascript_buffer;
EXTERNAL int javascript_buffer_len;

EXTERNAL void *Setl_Initialize();
EXTERNAL void compiler_init(SETL_SYSTEM_PROTO_VOID);
EXTERNAL void setl_set_io(FILE *_stdin, FILE *_stdout, FILE *_stderr);
EXTERNAL void giveup(SETL_SYSTEM_PROTO char *format, ...);
EXTERNAL void Setl_SetCommandLine(SETL_SYSTEM_PROTO int argc,int optind,char **argv);

EXTERNAL void Setl_SetVerboseMode(SETL_SYSTEM_PROTO int mode);
EXTERNAL void Setl_InitInterpreter(SETL_SYSTEM_PROTO_VOID);
EXTERNAL void Setl_PrintVersion(SETL_SYSTEM_PROTO_VOID);
EXTERNAL int set_lib_file(SETL_SYSTEM_PROTO char *f);
EXTERNAL int set_lib_path(SETL_SYSTEM_PROTO char *f);
EXTERNAL int set_compiler_options(SETL_SYSTEM_PROTO char *option,void *flag);
EXTERNAL void runtime_cleanup(SETL_SYSTEM_PROTO_VOID);
EXTERNAL int plugin_main(SETL_SYSTEM_PROTO const char *fileName);
EXTERNAL int execute_go(SETL_SYSTEM_PROTO int forever);
EXTERNAL void profiler_dump(SETL_SYSTEM_PROTO_VOID);
EXTERNAL void create_lib_file(SETL_SYSTEM_PROTO const char *file);
EXTERNAL void add_file_unit(SETL_SYSTEM_PROTO char *file_name, char *unit_name);

typedef void *filelist_ptr_type;
EXTERNAL void *setl_get_filelist(SETL_SYSTEM_PROTO char *specifier_list);
EXTERNAL void *setl_get_next_file(filelist_ptr_type current_file);
EXTERNAL char *setl_get_filename(filelist_ptr_type current_file);
EXTERNAL char *setl_free_filelist(filelist_ptr_type current_file);
EXTERNAL int setl_total_error_count(SETL_SYSTEM_PROTO_VOID);
EXTERNAL int setl_total_warning_count(SETL_SYSTEM_PROTO_VOID);
EXTERNAL void setl_set_fname(SETL_SYSTEM_PROTO const char *file);
EXTERNAL int compile_fragment(SETL_SYSTEM_PROTO const char *program_source, int compile_flag);
EXTERNAL int get_num_errors(SETL_SYSTEM_PROTO_VOID);
EXTERNAL int get_num_warnings(SETL_SYSTEM_PROTO_VOID);
EXTERNAL char	*get_err_string(SETL_SYSTEM_PROTO int i);

#endif //SETL_IDE


#ifdef __cplusplus
}
#endif

#endif



