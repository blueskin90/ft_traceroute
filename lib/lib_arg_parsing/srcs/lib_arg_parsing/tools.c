#include "lib_arg_parsing_internal.h"
#include <stdlib.h>
#include <strings.h>

t_flag* alloc_flag(void)
{
	t_flag *flag = NULL;

	flag = (t_flag*)malloc(sizeof(t_flag));
	if (!flag)
		return NULL;
	bzero(flag, sizeof(t_flag));
	return flag;
}

void	free_flag(t_flag *flag)
{
	free(flag->longname);
	free(flag->arg_name);
	free(flag->desc);
	free(flag);
}

int	add_to_list(t_flag *flag, t_flag **list)
{
	t_flag *ptr;

	if (list == NULL)
		return ERROR;
	if (*list == NULL)
		*list = flag;
	else {
		ptr = *list;
		while (ptr->next)
			ptr = ptr->next;
		ptr->next = flag;
	}
	return SUCCESS;
}

void free_list(t_flag *list)
{
	t_flag *ptr;

	while (list){
		ptr = list->next;
		free_flag(list);
		list = ptr;
	}
}
