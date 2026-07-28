#ifndef LIB_ARG_PARSING_INTERNAL_H
#define LIB_ARG_PARSING_INTERNAL_H

#include <stdint.h>
#include <stddef.h>
#include "lib_arg_parsing_structs.h"
#include "lib_arg_parsing.h"

#define TRUE 1
#define FALSE 0
#define USAGE_DESC "print usage and exit."
#define HELP_DESC "print help and exit."

enum e_type {
	UINT8_T = 0x1,
	UINT16_T = 0x2,
	UINT32_T = 0x4,
	UINT64_T = 0x8,
	INT8_T = 0x10,
	INT16_T = 0x20,
	INT32_T = 0x40,
	INT64_T = 0x80,
	STRING = 0x100,
	CHAR = 0x200,
	BOOL = 0x400,
	CUSTOM = 0x800,
	HELP_TYPE = 0x1000,
	USAGE_TYPE = 0x2000,
	FLOAT = 0x4000,
};

#define UNSIGNED_TYPE ((UINT8_T | UINT16_T | UINT32_T | UINT64_T))
#define SIGNED_TYPE ((INT8_T | INT16_T | INT32_T | INT64_T))

enum e_flags {
	HAS_DEFAULT = 0x1,
	HAS_MIN = 0x2,
	HAS_MAX = 0x4,
	IS_CUSTOM = 0x8,
    WAS_PARSED = 0x10,
};

enum e_parsing_step {
    FLAGS_PARSING,
    ARGS_PARSING,
    OPT_ARGS_PARSING
};

t_flag *alloc_flag(void);
void free_flag(t_flag *flag);
int add_to_list(t_flag *flag, t_flag **list);
void free_list(t_flag *list);
t_flag *get_first_unparsed_arg(t_flag *list);

t_flag *get_flag_by_long(char *flag, int len, t_flag *list);
t_flag *get_flag_by_short(char flag, t_flag *list);
t_flag *get_arg_by_name(char *name, t_flag *list);

int usage(int err);
void print_usage_line(void);
int parsing_error(int err, char *flag, char *arg, t_flag *flag_node);

int unknown_long_flag_error(int ac, char *flag, char *arg);
int unknown_short_flag_error(int ac, char flag);
int missing_arg_long(int ac, t_flag *flag);
int missing_arg_short(int ac, t_flag *flag);
int extra_arguments(int i, int ac, char **av);
int invalid_argument(t_flag *flag, char *arg);
int incorrect_value_signed(t_flag *flag, char *arg);
int incorrect_value_unsigned(t_flag *flag, char *arg);
int missing_mandatory(void);

int unsigned_check(char *val, struct s_flag *flag);
int signed_check(char *val, struct s_flag *flag);
int string_check(char *val, struct s_flag *flag);
int char_check(char *val, struct s_flag *flag);
int dummy_check(char *val, struct s_flag *flag);
int float_check(char *val, struct s_flag *flag);

int unsigned_parse(char *val, struct s_flag *flag);
int signed_parse(char *val, struct s_flag *flag);
int string_parse(char *val, struct s_flag *flag);
int char_parse(char *val, struct s_flag *flag);
int bool_parse(char *dummy, struct s_flag *flag);
int dummy_parse(char *val, struct s_flag *flag);
int help_parse(char *val, struct s_flag *flag);
int usage_parse(char *val, struct s_flag *flag);
int float_parse(char *val, struct s_flag *flag);

#endif
