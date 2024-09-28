#include "lib_arg_parsing_internal.h"
#include <stdio.h>
#include <string.h>



int incorrect_value_unsigned(t_flag *flag, char *arg)
{
	fprintf(stderr, "%s: ", env.progname);
	fprintf(stderr, "invalid argument: '%s': out of range: %lu <= value <= %lu\n", arg, flag->complementary.unsigned_integer.min, flag->complementary.unsigned_integer.max);
	return INCORRECT_VALUE;
}

int incorrect_value_signed(t_flag *flag, char *arg)
{
	fprintf(stderr, "%s: ", env.progname);
	fprintf(stderr, "invalid argument: '%s': out of range: %ld <= value <= %ld\n", arg, flag->complementary.integer.min, flag->complementary.integer.max);
	return INCORRECT_VALUE;
}

int invalid_argument(t_flag *flag, char *arg)
{
	fprintf(stderr, "%s: ", env.progname);
	fprintf(stderr, "invalid argument: `%s' for flag ", arg);
	if (flag->shortname[0] != 0) {
		fprintf(stderr, "-%c", flag->shortname[0]);
		if (flag->longname)
			fprintf(stderr, " or ");
	}
	if (flag->longname)
		fprintf(stderr, "--%s\n", flag->longname);
	return INVALID_ARGUMENT;
}

int extra_arguments(int i, int ac, char **av)
{
    int starting = i;
    fprintf(stderr, "%s: ", env.progname);
    if (i+1 == ac) {
        fprintf(stderr, "Extra arg `%s' (argc %d)\n", av[i], i);
    }
    else {
        fprintf(stderr, "Extra args:");
        while (i < ac) {
            if (starting == i)
                fprintf(stderr, " `%s' (argc %d)", av[i], i);
            else if (i+1 < ac)
                fprintf(stderr, ", `%s' (argc %d)", av[i], i);
            else
                fprintf(stderr, " and `%s' (argc %d)", av[i], i);
            i++;
        }
        fprintf(stderr, "\n");
    }
    return TOO_MANY_ARGS;
}

int unknown_long_flag_error(int ac, char *flag, char *arg) {
	fprintf(stderr, "%s: ", env.progname);
	fprintf(stderr, "Bad option `--%s`",flag);
	if (arg)
		fprintf(stderr, " (with arg `%s')", arg);
	fprintf(stderr, " (argc %d)\n", ac);
	return UNKNOWN_FLAG;	
}

int unknown_short_flag_error(int ac, char flag)
{
	fprintf(stderr, "%s: ", env.progname);
	fprintf(stderr, "Bad option `-%c' (argc %d)\n", flag, ac);
	return UNKNOWN_FLAG;
}

int missing_arg_long(int ac, t_flag *flag)
{
    fprintf(stderr, "%s: ", env.progname);
    fprintf(stderr, "Missing argument for flag --%s (argc %d), should be in the shape of: --%s=ARGUMENT\n", flag->longname, ac, flag->longname);
    return MISSING_ARG;
}

int missing_arg_short(int ac, t_flag *flag)
{
    fprintf(stderr, "%s: ", env.progname);
    fprintf(stderr, "Missing argument for flag -%c (argc %d)\n",  flag->shortname[0], ac);
    return MISSING_ARG;
}

int missing_mandatory(void)
{
	t_flag *flag;

	flag = get_first_unparsed_arg(env.args);
    fprintf(stderr, "%s: ", env.progname);
	fprintf(stderr, "Missing mandatory argument `%s'\n", flag->arg_name);
	return MISSING_MANDATORY;
}

void print_boolean_flags(void)
{
	int encountered = 0;
	t_flag *ptr;

	ptr = env.flags;
	while (ptr) {
		if (ptr->type == BOOL && ptr->shortname[0] != 0) {
			if (encountered == 0) {
				fprintf(stderr, "[ -");
				encountered = 1;
			}
			fprintf(stderr, "%c", ptr->shortname[0]);
		}
		ptr = ptr->next;
	}
	if (encountered == 1)
		fprintf(stderr, " ] ");
}

void print_short_flags(void)
{
	t_flag *ptr;

	ptr = env.flags;
	while (ptr) {
		if (ptr->type != BOOL && ptr->shortname[0] != 0) {
			if (ptr->type == HELP_TYPE || ptr->type == USAGE_TYPE)
				fprintf(stderr, "[ -%c ] ", ptr->shortname[0]);
			else
				fprintf(stderr, "[ -%c %s ] ", ptr->shortname[0], ptr->arg_name);
		}
		ptr = ptr->next;
	}
}

void print_long_flags(void)
{
	t_flag *ptr;

	ptr = env.flags;
	while (ptr) {
		if (ptr->shortname[0] == 0) {
			if (ptr->arg_name)
				fprintf(stderr, "[ --%s=%s ] ", ptr->longname, ptr->arg_name);
			else
				fprintf(stderr, "[ --%s ] ", ptr->longname);
		}
		ptr = ptr->next;
	}
}

void print_args(void)
{
	t_flag *ptr;

	ptr = env.args;
	while (ptr) {
		fprintf(stderr, "%s ", ptr->arg_name);
		ptr = ptr->next;
	}
}

void print_opt_args(void)
{
	t_flag *ptr;

	ptr = env.opt_args;
	while (ptr) {
		fprintf(stderr, "[ %s ]", ptr->arg_name);
		ptr = ptr->next;
	}
}

void print_usage_line(void)
{
	fprintf(stderr, "Usage\n    %s ", env.progname);
	print_boolean_flags();
	print_short_flags();
	print_long_flags();
	print_args();
	print_opt_args();
	fprintf(stderr, "\n");
}

int is_whitespace(char c)
{
	return (c == ' ' || c == '\n' || c == '\t');
}

int get_word_size(char *str)
{
	int i = 0;

	while (str[i] && !is_whitespace(str[i]))
			i++;
	return i;
}

void strcpywrd_max(char *dst, char **src, size_t len)
{
	size_t i = 0;
	size_t tmp = 0;
	
	while (is_whitespace(**src))
			(*src)++;
	while ((*src)[i] && i < len)
	{
		if (is_whitespace((*src)[i])) {
			dst[i] = (*src)[i];
			i++;
		}
		else {
			tmp = get_word_size((*src) + i);
			if (i + tmp < len)
				strncpy(dst + i, *src + i, tmp);
			else
				break;
			i += tmp;
		}
	}
	*src += i;
	dst[i] = 0;
}

void print_min_max_default_signed(t_flag *flag, int charnum)
{
	fprintf(stderr, "%*s(min: %ld, max: %ld", charnum, "", flag->complementary.integer.min, flag->complementary.integer.max);
	if (flag->flags & HAS_DEFAULT)
		fprintf(stderr, ", default: %ld", flag->complementary.integer.default_val);
	fprintf(stderr, ")\n");

}

void print_min_max_default_unsigned(t_flag *flag, int charnum)
{
	fprintf(stderr, "%*s(min: %lu, max: %lu", charnum, "", flag->complementary.unsigned_integer.min, flag->complementary.unsigned_integer.max);
	if (flag->flags & HAS_DEFAULT)
		fprintf(stderr, ", default: %lu", flag->complementary.unsigned_integer.default_val);
	fprintf(stderr, ")\n");
}

void print_min_max_default(t_flag *flag, int charnum)
{
	if (flag->type & UNSIGNED_TYPE)
		print_min_max_default_signed(flag, charnum);
	else if (flag->type & SIGNED_TYPE)
		print_min_max_default_unsigned(flag, charnum);
}

/*
** makes the desc esaily readable by aligning it to where the previous desc was
*/
void print_desc(t_flag *ptr, int charnum, char *desc)
{
    char buffer[81];

    if (!desc) {
	if (ptr->type == USAGE_TYPE)
		desc = USAGE_DESC;
	else if (ptr->type == HELP_TYPE)
		desc = HELP_DESC;
	else
        	return;
    }
    for (int i = 0; i < 80; i++)
	    buffer[i] = ' ';
    buffer[80] = 0;
    if (charnum >= 29) {
        fprintf(stderr, "\n");
        charnum = 30;
    }
    else {
		strcpywrd_max(buffer + charnum + 1, &desc, 79 - (charnum + 1));
        fprintf(stderr, "%s\n", buffer + charnum);
		charnum++;
    }
    while (*desc) {
		strcpywrd_max(buffer + charnum, &desc, 79 - (charnum));
        fprintf(stderr, "%s\n", buffer);
    }
    print_min_max_default(ptr, charnum);
}

void print_options(void)
{
	t_flag *ptr;
	int size;

	if (!env.flags)
		return;
	ptr = env.flags;
	fprintf(stderr, "Options:\n");
	while (ptr) {
		size = 0;
		size += fprintf(stderr, "  ");
		if (ptr->shortname[0]) {
			size += fprintf(stderr, "-%c ", ptr->shortname[0]);
			if (ptr->arg_name)
				size += fprintf(stderr, " %s ", ptr->arg_name);
			if (ptr->longname)
				size += fprintf(stderr, " ");
		}
		if (ptr->longname) {
			size += fprintf(stderr, "--%s", ptr->longname);
			if (ptr->arg_name)
				size += fprintf(stderr, "=%s", ptr->arg_name);
			print_desc(ptr, size, ptr->desc);
		}
		fprintf(stderr, "\n");
		ptr = ptr->next;
	}
    fprintf(stderr, "\n");
}

void print_arg(void)
{
	t_flag *ptr = env.args;
	int size;

	if (ptr == NULL)
		return;
	fprintf(stderr, "Arguments:\n");
	while (ptr) {
		size = 0;
		size += fprintf(stderr, "  %s", ptr->arg_name);
		print_desc(ptr, size, ptr->desc);
		ptr = ptr->next;
	}
    fprintf(stderr, "\n");
}

void print_opt_arg(void)
{
	t_flag *ptr = env.opt_args;
	int size;

	if (ptr == NULL)
		return;
	fprintf(stderr, "Optional arguments:\n");
	while (ptr) {
		size = 0;
		size += fprintf(stderr, "  <%s>", ptr->arg_name);
		print_desc(ptr, size, ptr->desc);
		ptr = ptr->next;
	}
}

int usage(int err)
{
	print_usage_line();
	print_options();
	print_arg();
	print_opt_arg();
	return err;
}

int parsing_error(int err, char *flag, char *arg, t_flag *flag_node)
{
	(void)flag;
	(void)arg;
	(void)flag_node;
	return err;
}
