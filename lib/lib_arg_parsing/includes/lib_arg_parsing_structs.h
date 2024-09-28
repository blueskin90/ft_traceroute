#ifndef LIB_ARG_PARSING_STRUCTS_H
#define LIB_ARG_PARSING_STRUCTS_H

#include <stdint.h>
#include <stddef.h>

typedef struct s_uint {
	uint64_t min;
	uint64_t max;
	uint64_t default_val;
} t_uint;

typedef struct s_int {
	int64_t min;
	int64_t max;
	int64_t default_val;
} t_int;

typedef struct s_bool {
	uint64_t mask;
	size_t ptr_size;
} t_bool;

union u_integers {
		t_int integer;
		t_uint unsigned_integer;
		t_bool bool_values;
};

typedef struct s_flag {
	char shortname[1];
	char *longname;
	char *arg_name;
	char *desc;
	void *data;
	int (*check)(char*, struct s_flag *);
	int (*parse)(char*, struct s_flag *);
	uint64_t type;
	uint64_t flags;
	union u_integers complementary;
	struct s_flag *next;
} t_flag;

typedef struct s_parse_env {
	char *progname;
	char *progname_short;
	t_flag *flags; // not mandatory, order not important
	t_flag *args; // mandatory, order important
	t_flag *opt_args; // not mandatory, order important
} t_parse_env;

#endif
