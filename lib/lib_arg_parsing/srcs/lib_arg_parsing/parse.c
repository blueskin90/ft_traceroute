#include "lib_arg_parsing_internal.h"
#include <string.h>
#include <stdio.h>

t_parse_env env;

int list_was_parsed(t_flag *list)
{
    while (list) {
        if ((list->flags & WAS_PARSED) == 0) 
            return ERROR;
        list = list->next;
    }
    return SUCCESS;
}

char* get_progname_short(char *name)
{
    char *shortname = name + strlen(name);

    while (shortname > name && *shortname != '/')
        shortname--;
    if (*shortname == '/')
        shortname++;
    return shortname;
}

t_flag *get_arg_by_name(char *name, t_flag *list)
{
    while (list){
        if (strncmp(name, list->arg_name, strlen(name)) == 0)
            return list;
        list = list->next;
    }
    return NULL;
}

t_flag *get_flag_by_long(char *flag, int len, t_flag *list)
{
    while (list){
        if (strncmp(flag, list->longname, len) == 0)
            return list;
        list = list->next;
    }
    return NULL;
} 

t_flag *get_flag_by_short(char flag, t_flag *list)
{
    while (list){
        if (list->shortname[0] == flag)
            return list;
        list = list->next;
    }
    return NULL;
}

// penser a accepter -abcd si les 4 sont des bool, et meme -abcd pwet si d prend un argument et c'est pwet
int parse_short_flag(int *i, int ac, char **av)
{
    char *flag = av[*i] + 1;
    char *arg;
    t_flag *flag_node;
    int retval;

    if (flag[0] == 0)
        return unknown_short_flag_error(*i, *flag);
    flag_node = get_flag_by_short(*flag, env.flags);
    if (flag_node == NULL)
        return unknown_short_flag_error(*i, *flag);
    while (flag_node->type == BOOL || flag_node->type == HELP_TYPE || flag_node->type == USAGE_TYPE) {
	retval = flag_node->parse(NULL, flag_node);
	if (retval != SUCCESS)
	    return retval;
        flag_node->flags |= WAS_PARSED;
        flag++;
        if (flag[0] == 0)
            return SUCCESS;
        flag_node = get_flag_by_short(*flag, env.flags);
        if (flag_node == NULL)
            return unknown_short_flag_error(*i, *flag);
    }
    arg = flag + 1;
    if (arg[0] == 0) {
        if (*i + 1 < ac) {
            (*i)++;
            arg = av[*i];
        }
        else
            return missing_arg_short(*i, flag_node);
    }
    while (*arg != 0 && *arg == ' ')
        arg++;
    retval = flag_node->check(arg, flag_node);
    if (retval != SUCCESS)
        return retval;
    retval = flag_node->parse(arg, flag_node);
    if (retval != SUCCESS)
        return retval;
    flag_node->flags |= WAS_PARSED;
    return SUCCESS;

}

int parse_long_flag(int *i, int ac, char **av)
{
    char *flag = av[*i] + 2;
    char *arg = strchr(av[*i], '=');
    t_flag *flag_node;
    int retval;

    if (arg)
        arg++;
    (void)ac;
    if (arg == NULL) {
        flag_node = get_flag_by_long(flag, strlen(flag), env.flags);
        if (flag_node == NULL)
            return unknown_long_flag_error(*i, flag, arg);
        if (flag_node->type != BOOL && flag_node->type != HELP_TYPE && flag_node->type != USAGE_TYPE)
            return missing_arg_long(*i, flag_node); // verifier si forme == --pwet 1234 ? ou assume c'est tjr --pwet=1234 ?
	flag_node->parse(NULL, flag_node);
        flag_node->flags |= WAS_PARSED;
        return SUCCESS;
    }
    flag_node = get_flag_by_long(flag, (int)(arg - flag), env.flags);
    if (flag_node == NULL)
        return unknown_long_flag_error(*i, flag, arg);
    retval = flag_node->check(arg, flag_node);
    if (retval != SUCCESS)
        return retval;
    retval = flag_node->parse(arg, flag_node);
    if (retval != SUCCESS)
        return retval;
    flag_node->flags |= WAS_PARSED;
    return SUCCESS;
}

int parse_flag(int *i, int ac, char **av)
{
    if (av[*i][0] == '-') {
        if (av[*i][1] =='-')
            return parse_long_flag(i, ac, av);
        else
            return parse_short_flag(i, ac, av);
    }
    return ERROR;
}

t_flag *get_first_unparsed_arg(t_flag *list)
{
    while (list) {
        if ((list->flags & WAS_PARSED) == 0)
            return list;
        list = list->next;
    }
    return NULL;
}

int parse_arg(char *arg)
{
    t_flag *to_parse = get_first_unparsed_arg(env.args);
    int retval;

    if (to_parse == NULL)
        return ERROR;
    retval = to_parse->check(arg, to_parse);
    if (retval != SUCCESS)
        return retval;
    retval = to_parse->parse(arg, to_parse);
    to_parse->flags |= WAS_PARSED;
    return retval;
}

int parse_opt_arg(char *arg)
{
    t_flag *to_parse = get_first_unparsed_arg(env.opt_args);
    int retval;

    if (to_parse == NULL)
        return ERROR;
    retval = to_parse->check(arg, to_parse);
    if (retval != SUCCESS)
        return retval;
    retval = to_parse->parse(arg, to_parse);
    to_parse->flags |= WAS_PARSED;
    return retval;
}

int parse(int ac, char **av)
{
    int step = FLAGS_PARSING;
    int retval;
    int i = 0;

    env.progname = av[0];
    env.progname_short = get_progname_short(av[0]);
    step = FLAGS_PARSING;
    i = 1;
    while (i < ac)
    {
        if (step == FLAGS_PARSING) {
            if (av[i][0] != '-') {
                step = ARGS_PARSING;
                continue;
            }
            else {
                if (av[i][0] == '-' && av[i][1] == 0) {
                    step = ARGS_PARSING;
                    continue;
                }
                retval = parse_flag(&i, ac, av);
                if (retval != SUCCESS)
                    return retval;
                i++;
            }
        }
        else if (step == ARGS_PARSING) {
            retval = parse_arg(av[i]);
            if (list_was_parsed(env.args) == SUCCESS)
                step = OPT_ARGS_PARSING;
            i++;
        }
        else if (step == OPT_ARGS_PARSING) {
            if (list_was_parsed(env.opt_args) == SUCCESS)
                return extra_arguments(i, ac, av); 
            retval = parse_opt_arg(av[i]);
            if (retval != SUCCESS)
                return retval;
            i++;
        }
        else {
            printf("error in step parsing");
            return ERROR;
        }
    }
    if (list_was_parsed(env.args) != SUCCESS)
	    return missing_mandatory(); 
    return SUCCESS;
}
