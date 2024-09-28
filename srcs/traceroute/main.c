#include "ft_traceroute.h"
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include "lib_arg_parsing.h"

void			dump_env(struct s_env *env)
{
	printf("max_ttl : %hhu\n", env->args.max_ttl);
	printf("start_ttl : %hhu\n", env->args.start_ttl);
	printf("probe_per_hop : %hhu\n", env->args.probe_per_hop);
	printf("send_wait : %u\n", env->args.send_wait);
	printf("packet_len : %u\n", env->args.packet_len);
	printf("protocol type: %hhx\n", env->args.protocol_type);
	printf("flags: %hhx\n", env->args.flags);
}

static int		free_env(struct s_env *env)
{
	(void)env;
	return SUCCESS;
}

static int		init_env(struct s_env *env)
{
	bzero(env, sizeof(struct s_env));
	env->args.packet_len = DEFAULT_DATA_LEN;
	env->args.max_ttl = DEFAULT_MAX_TTL;
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
	retval = init_lib("config/config.ntmlp");
	if (retval != SUCCESS)
		return retval;
	set_ptr(&env.args.start_ttl, "-f");
	set_ptr(&env.args.max_ttl, "-m");
	set_ptr(&env.args.protocol.tcp.dport, "-p");
	set_ptr(&env.args.probe_per_hop, "-q");
	set_string_ptr(&env.args.dest, "host");
	set_ptr(&env.args.packet_len, "packetlen");
	set_bool_ptr_mask(&env.args.flags, sizeof(char), MTU_FLAG, "--mtu");
	retval = parse(ac, av);	
	if (retval != SUCCESS)
		return retval;
	dump_env(&env);
	close_lib();
	free_env(&env);
	return 0;
}

