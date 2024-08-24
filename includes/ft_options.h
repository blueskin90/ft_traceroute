#ifndef FT_OPTIONS_H
# define FT_OPTIONS_H

#define LONG_NAME_MAX 15
#define PARAM_NAME_MAX 15

struct s_option {
	char short_name;
	char[LONG_NAME_MAX] long_name;
	char[PARAM_NAME_MAX] param_name;
}

#endif
