#ifndef LIB_ARG_PARSING_H
#define LIB_ARG_PARSING_H
#include "lib_arg_parsing_structs.h"

enum e_errorcode {
	ERROR,
	SUCCESS,
	PARSING_ERROR,
	INVALID_ARGUMENT,
	UNKNOWN_FLAG,
	MISSING_ARG,
	ALREADY_PARSED,
	USAGE,
	INCORRECT_VALUE,
	MISSING_MANDATORY,
    TOO_MANY_ARGS,
};

extern t_parse_env env;

int init_lib(char *path);
int parse(int ac, char **av);
int close_lib(void);

int set_bool_ptr_mask(void *ptr, size_t size, uint64_t mask, char *flagstr);
int set_ptr(void *ptr, char *flagstr);
int set_string_ptr(char **ptr, char *flagstr);

#endif
