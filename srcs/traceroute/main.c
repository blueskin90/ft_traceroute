#include "ft_traceroute.h"
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

static int		free_env(struct s_env *env)
{
	return SUCCESS;
}

static int		init_env(struct s_env *env)
{
	bzero(&env, sizeof(env));
	env->args.data_size = DEFAULT_DATA_LEN;
	env->args.ttl = DEFAULT_MAX_TTL;
	env->args.start_ttl = DEFAULT_START_TTL;
	env->args.probe_per_hop = DEFAULT_PROBE_PER_HOP;
	env->args.send_wait = DEFAULT_SEND_WAIT;
	env->args.protocol_type = DEFAULT_PROTOCOL;
	return SUCCESS;
}

int			main(int ac, char **av)
{
	struct s_env env;
	int retval;

	retval = init_env(&env);
	if (retval != SUCCESS)
		return retval;
	retval = args_parsing(&env, ac, av);
	if (retval != SUCCESS)
		return retval;
	free_env(&env);
	return 0;
}

