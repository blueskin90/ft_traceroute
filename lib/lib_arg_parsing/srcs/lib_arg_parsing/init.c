#include "lib_arg_parsing_internal.h"
#include <stdio.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

char    *clean_line(char *line)
{
    // cut comment and delete "\n"

    for (int i = 0; line[i] != 0; ++i)
    {
        if (line[i] == '#')
        {
            while (line[i] == '#' || line[i] == ' ' || line[i] == '\t')
                --i;
            ++i;
            line[i] = '\n';
            line[i + 1] = '\0';
            break;
        }
    }

    if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = '\0';

    return line;
}

char    *skip_empty(FILE *stream, int *line_nb)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
        
    if ((nread = getline(&line, &len, stream)) < 0)
    {
        fprintf(stderr, "err(3): invalid line at line %d\n", *line_nb);
        return NULL;
    }
    ++(*line_nb);

    while (strcmp("", line) == 0 || strcmp("\t", line) == 0
        || strcmp("    ", line) == 0)
    {
        free(line);
        if ((nread = getline(&line, &len, stream)) < 0)
        {
            fprintf(stderr, "err(3): invalid line at line %d\n", *line_nb);
            return NULL;
        }
        ++(*line_nb);
    }

    return line;
}

int     zone_check(FILE *stream, int *line_nb, char* zonename)
{
    char *line = NULL;
    char *zoneline;

    if ((line = skip_empty(stream, line_nb)) == NULL)
        return ERROR;
    clean_line(line);

    zoneline = (char*)malloc(strlen(zonename) + strlen(":") + 1);
    strcpy(zoneline, zonename);
    strcat(zoneline, ":");
    if (strcmp(zoneline, line) != 0)
    {
        fprintf(stderr, "err(2): invalid zone: [%s] vs [%s]\n", zoneline, line);
        free(zoneline);
        free(line);
        return ERROR;
    }
    free(line);
    free(zoneline);

    return SUCCESS;
}

int    parse_descriptor_line(char *line, int *line_nb, t_flag *current)
{
    const char shortname_format[] = "\tshort: ";
    const char longname_format[] = "\tlong: ";
    const char arg_name_format[] = "\targ_name: ";
    const char type_format[] = "\ttype: ";
    const char *type_table[] = {
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "int8_t",   "int16_t",   "int32_t",   "int64_t",
        "string",   "char",      "bool",      "custom",
	"help", "usage",
	NULL
    };
    const char desc_format[] = "\tdesc: ";
    const char default_format[] = "\tdefault: ";
    const char min_format[] = "\tmin: ";
    const char max_format[] = "\tmax: ";

    if (strncmp(shortname_format, line, strlen(shortname_format)) == 0)            
        current->shortname[0] = line[strlen(shortname_format)];
    else if (strncmp(longname_format, line, strlen(longname_format)) == 0)            
        current->longname = strdup(line + strlen(longname_format));
    else if (strncmp(arg_name_format, line, strlen(arg_name_format)) == 0)            
        current->arg_name = strdup(line + strlen(arg_name_format));
    else if (strncmp(type_format, line, strlen(type_format)) == 0)            
    {
        for (int i = 0; type_table[i] != NULL; ++i)
        {
            if (strcmp(line + strlen(type_format), type_table[i]) == 0)
            {
                current->type = 0x1 << i;
                return SUCCESS;
            }
        }
        fprintf(stderr, "err(6): invalide type : [%s] at line %d\n",
            line + strlen(type_format), *line_nb);
        return ERROR;
    }
    else if (strncmp(desc_format, line, strlen(desc_format)) == 0)            
        current->desc = strdup(line + strlen(desc_format));
    else if (strncmp(default_format, line, strlen(default_format)) == 0)
    {
        if (current->type >= UINT8_T && current->type <= UINT64_T)
            current->complementary.unsigned_integer.default_val =
                atoi(line + strlen(default_format));
        else if (current->type >= INT8_T && current->type <= INT64_T)
            current->complementary.integer.default_val =
                atoi(line + strlen(default_format));
        else if (current->type == 0)
        {
            fprintf(stderr, "err(10): no type given for default at line %d\n", *line_nb);
            return ERROR;       
        }
        else
        {
            fprintf(stderr, "err(10): invalid type for default at line %d\n", *line_nb);
            return ERROR;       
        }
        current->flags |= HAS_DEFAULT;
    }
    else if (strncmp(min_format, line, strlen(min_format)) == 0)            
    {
        if (current->type >= UINT8_T && current->type <= UINT64_T)
            current->complementary.unsigned_integer.min =
                atoi(line + strlen(min_format));
        else if (current->type >= INT8_T && current->type <= INT64_T)
            current->complementary.integer.min =
                atoi(line + strlen(min_format));
        else if (current->type == 0)
        {
            fprintf(stderr, "err(10): no type given for min at line %d\n", *line_nb);
            return ERROR;       
        }
        else
        {
            fprintf(stderr, "err(10): invalid type for min at line %d\n", *line_nb);
            return ERROR;       
        }
        current->flags |= HAS_MIN;
    }
    else if (strncmp(max_format, line, strlen(max_format)) == 0)            
    {
        if (current->type >= UINT8_T && current->type <= UINT64_T)
            current->complementary.unsigned_integer.max =
                atoi(line + strlen(max_format));
        else if (current->type >= INT8_T && current->type <= INT64_T)
            current->complementary.integer.max =
                atoi(line + strlen(max_format));
        else if (current->type == 0)
        {
            fprintf(stderr, "err(10): no type given for max at line %d\n", *line_nb);
            return ERROR;       
        }
        else
        {
            fprintf(stderr, "err(10): invalid type for max at line %d\n", *line_nb);
            return ERROR;       
        }
        current->flags |= HAS_MAX;
    }
    else
    {
        fprintf(stderr, "err(7): invalid flag descriptor at line %d\n", *line_nb);
        return ERROR;
    }
    return SUCCESS;
}

void    assign_min_max(t_flag *current)
{
    if ((current->flags & HAS_MIN) == 0)
    {
        switch (current->type)
        {
            case UINT8_T: current->complementary.unsigned_integer.min = 0; break;
            case UINT16_T: current->complementary.unsigned_integer.min = 0; break;
            case UINT32_T: current->complementary.unsigned_integer.min = 0; break;
            case UINT64_T: current->complementary.unsigned_integer.min = 0; break;
            case INT8_T: current->complementary.integer.min = CHAR_MIN; break;
            case INT16_T: current->complementary.integer.min = SHRT_MIN; break;
            case INT32_T: current->complementary.integer.min = INT_MIN; break;
            case INT64_T: current->complementary.integer.min = LONG_MIN; break;
            case CHAR: current->complementary.integer.min = 0; break;
            default: current->complementary.unsigned_integer.min = 0;
        }
    }
    if ((current->flags & HAS_MAX) == 0)
    {
        switch (current->type)
        {
            case UINT8_T: current->complementary.unsigned_integer.max = UCHAR_MAX; break;
            case UINT16_T: current->complementary.unsigned_integer.max = USHRT_MAX; break;
            case UINT32_T: current->complementary.unsigned_integer.max = UINT_MAX; break;
            case UINT64_T: current->complementary.unsigned_integer.max = ULONG_MAX; break;
            case INT8_T: current->complementary.integer.max = CHAR_MAX; break;
            case INT16_T: current->complementary.integer.max = SHRT_MAX; break;
            case INT32_T: current->complementary.integer.max = INT_MAX; break;
            case INT64_T: current->complementary.integer.max = LONG_MAX; break;
            case CHAR: current->complementary.integer.max = UCHAR_MAX; break;
            default: current->complementary.unsigned_integer.max = ULONG_MAX;
        }
    }
}

void    assign_check_function(t_flag *current)
{
    if (current->type & UNSIGNED_TYPE)
        current->check = &unsigned_check;
    else if (current->type & SIGNED_TYPE)
        current->check = &signed_check;
    else if (current->type & STRING)
        current->check = &string_check;
    else if (current->type & CHAR)
        current->check = &char_check;
    else
        current->check = &dummy_check;
}

void    assign_parse_function(t_flag *current)
{
    if (current->type & UNSIGNED_TYPE)
        current->parse = &unsigned_parse;
    else if (current->type & SIGNED_TYPE)
        current->parse = &signed_parse;
    else if (current->type & STRING)
        current->parse = &string_parse;
    else if (current->type & CHAR)
        current->parse = &char_parse;
    else if (current->type & BOOL)
        current->parse = &bool_parse;
    else if (current->type & CUSTOM)
        current->parse = NULL;
    else if (current->type & HELP_TYPE)
	current->parse = &help_parse;
    else if (current->type & USAGE_TYPE)
	current->parse = &usage_parse;
    else // unused
        current->parse = &dummy_parse;
}

int     validate_flag(t_flag *current, int *line_nb)
{
    if (current->shortname[0] == 0 && current->longname == NULL)
    {
        fprintf(stderr, "err(4): missing longname or shortname at line %d\n", *line_nb);
        return ERROR;
    }
    if (current->type == 0)
    {
        fprintf(stderr, "err(11): missing type on line %d\n", *line_nb);
        return ERROR;
    }
    assign_min_max(current);
    assign_check_function(current);
    assign_parse_function(current);

    return SUCCESS;
}

int     parse_flags(FILE *stream, int *line_nb)
{
    char    *line = NULL;
    size_t  len = 0;
    ssize_t nread;
    t_flag  *current;
    int     empty_zone = TRUE;

    env.flags = alloc_flag();
    current = env.flags;
    while ((nread = getline(&line, &len, stream)) >= 0)
    {
        line = clean_line(line);
        // end of flag zone
        if (strcmp("", line) == 0 || strcmp("\t", line) == 0
                || strcmp("    ", line) == 0)
            break;
        // end of one flag section, verifying and adding new one
        else if (strcmp("\t-", line) == 0)
        {
            if (validate_flag(current, line_nb) == ERROR)
                return ERROR;
            current->next = alloc_flag();
            current = current->next;
        }        
        else if (parse_descriptor_line(line, line_nb, current) == ERROR)
            return ERROR;
        else 
            empty_zone = FALSE;
        
        ++(*line_nb);
    }
    if (empty_zone == FALSE && validate_flag(current, line_nb) == ERROR)
        return ERROR;
    free(line);

    return SUCCESS;
}

int     validate_argument(t_flag *current, int *line_nb)
{
    if (current->arg_name == NULL)
    {
        fprintf(stderr, "err(8): no name given to argument at line %d\n", *line_nb);
        return ERROR;
    }
    if (current->type == 0)
    {
        fprintf(stderr, "err(11): missing type on line %d\n", *line_nb);
        return ERROR;
    }
    assign_min_max(current);
    assign_check_function(current);
    assign_parse_function(current);

    return SUCCESS;
}

int     parse_arguments(FILE *stream, int *line_nb, t_flag **env_to_create)
{
    char    *line = NULL;
    size_t  len = 0;
    ssize_t nread;
    t_flag  *current;
    int     empty_zone = TRUE;

    *env_to_create = alloc_flag();
    current = *env_to_create;
    while ((nread = getline(&line, &len, stream)) >= 0)
    {
        line = clean_line(line);
        // check for end of flag zone
        if (strcmp("", line) == 0 || strcmp("\t", line) == 0
                || strcmp("    ", line) == 0)
            break;
        // end of one flag section, verifying and adding new one
        else if (strcmp("\t-", line) == 0)
        {
            if (validate_argument(current, line_nb) == ERROR)
                return ERROR;
            current->next = alloc_flag();
            current = current->next;
        }        
        else if (parse_descriptor_line(line, line_nb, current) == ERROR)
            return ERROR;
        else
            empty_zone = FALSE;
        ++(*line_nb);
    }
    if (empty_zone == FALSE && validate_argument(current, line_nb) == ERROR)
        return ERROR;
    free(line);

    return SUCCESS;
}

int parse_file(FILE *stream)
{
    // ici lire ligne par ligne et remplir le env.flags env.args ou env.opt_args
    char    flag_name[] = "flag";
    char    arg_name[] = "argument";
    char    opt_arg_name[] = "optional argument";
    int     line_nb = 0;

    if (zone_check(stream, &line_nb, flag_name) == ERROR
            || parse_flags(stream, &line_nb) == ERROR)
        return ERROR;
    if (zone_check(stream, &line_nb, arg_name) == ERROR
            || parse_arguments(stream, &line_nb, &env.args) == ERROR)
        return ERROR;
    if (zone_check(stream, &line_nb, opt_arg_name) == ERROR
            || parse_arguments(stream, &line_nb, &env.opt_args) == ERROR)
        return ERROR;

    return SUCCESS;
}

int	init_lib(char *config_path)
{
    FILE *stream;
    int out;

    bzero(&env, sizeof(t_parse_env));
    
    stream = fopen(config_path, "r");
    if (stream == NULL)
    {
        fprintf(stderr, "err(9): couldn't open file at %s\n", config_path);
        return ERROR;
    }

    out = parse_file(stream);

    fclose(stream);

    return out;
}
