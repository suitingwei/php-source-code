
/* $Id$ */

#ifndef CLI_H
#define CLI_H

#ifdef PHP_WIN32
#   define PHP_CLI_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#   define PHP_CLI_API __attribute__ ((visibility("default")))
#else
#   define PHP_CLI_API
#endif


extern PHP_CLI_API size_t sapi_cli_single_write(const char *str, size_t str_length);

typedef struct  {
	size_t (*cli_shell_write)(const char *str, size_t str_length);
	size_t (*cli_shell_ub_write)(const char *str, size_t str_length);
	int (*cli_shell_run)(void);
} cli_shell_callbacks_t;

extern PHP_CLI_API cli_shell_callbacks_t *php_cli_get_shell_callbacks();

#endif

