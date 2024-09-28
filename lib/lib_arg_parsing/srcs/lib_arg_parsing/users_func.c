#include "lib_arg_parsing_internal.h"
#include <stdio.h>
#include <string.h>

int set_string_ptr(char **ptr, char *flagstr)
{
	t_flag *flag = NULL;
	
	if (flagstr[0] == '-' && flagstr[1] == '-')
		flag = get_flag_by_long(flagstr + 2, strlen(flagstr + 2), env.flags);
	else if (flagstr[0] == '-')
		flag = get_flag_by_short(flagstr[1], env.flags);
	else {
		flag = get_arg_by_name(flagstr, env.args);
		if (!flag)
			flag = get_arg_by_name(flagstr, env.opt_args);
	}
	if (!flag)
		return UNKNOWN_FLAG;
	flag->data = ptr;
	return SUCCESS;
}

int set_ptr(void *ptr, char *flagstr)
{
	t_flag *flag = NULL;
	
	if (flagstr[0] == '-' && flagstr[1] == '-')
		flag = get_flag_by_long(flagstr + 2, strlen(flagstr + 2), env.flags);
	else if (flagstr[0] == '-')
		flag = get_flag_by_short(flagstr[1], env.flags);
	else {
		flag = get_arg_by_name(flagstr, env.args);
		if (!flag)
			flag = get_arg_by_name(flagstr, env.opt_args);
	}
	if (!flag)
		return UNKNOWN_FLAG;
	flag->data = ptr;
	return SUCCESS;
}

int set_bool_ptr_mask(void *ptr, size_t size, uint64_t mask, char *flagstr)
{
	t_flag *flag = NULL;

	if (flagstr[0] == '-' && flagstr[1] == '-')
		flag = get_flag_by_long(flagstr + 2, strlen(flagstr + 2), env.flags);
	else if (flagstr[0] == '-')
		flag = get_flag_by_short(flagstr[1], env.flags);
	if (!flag)
		return UNKNOWN_FLAG;
	flag->data = ptr;
	flag->complementary.bool_values.mask = mask;
	flag->complementary.bool_values.ptr_size = size;
	return SUCCESS;
}

int close_lib(void)
{
	free_list(env.flags);
	free_list(env.args);
	free_list(env.opt_args);
	return SUCCESS;
}


