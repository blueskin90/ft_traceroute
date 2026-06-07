#include "ft_traceroute.h"
#include <string.h>
#include <stdlib.h> // for the free ...
#include <unistd.h>

static int	init_params(struct s_params *params)
{
	bzero(params, sizeof(struct s_params)); /* to init the flags */
	params->max_ttl = DEFAULT_MAX_TTL;
	params->first_ttl = DEFAULT_FIRST_TTL;
	params->probe_per_hop = DEFAULT_PROBE_PER_HOP;
	params->probe_number = DEFAULT_PROBE_NUMBER;
	params->dest_port = DEFAULT_STANDARD_UDP_DEST_PORT;
	params->max_timeout = DEFAULT_MAX_TIMEOUT;
	params->here_factor = DEFAULT_HERE_FACTOR;
	params->near_factor = DEFAULT_NEAR_FACTOR;
	params->send_wait = DEFAULT_SEND_WAIT;
	params->packet_len = DEFAULT_PACKET_LEN;
	return SUCCESS;
}

static int	init_env(struct s_env *env, char *prog)
{
	env->pid = (uint16_t)getpid();
	env->prog = prog; 
	return SUCCESS;
}

static int	parsing(int ac, char **av, struct s_params *params)
{		
	(void)ac;
	(void)av;
	params->flags.icmp = 1;
	params->dest_port = DEFAULT_ICMP_SEQ;
	params->host = strdup("8.8.8.8"); // for temporary test without parsing
	// consider ./ft_traceroute -I 8.8.8.8
	// disallow any combination of flags for icmp / tcp / udp
	return SUCCESS;
}

int	traceroute_icmp(struct s_env *env, struct s_params *params)
{
	printf("to implement !");
	return FAILURE;
}

int	traceroute_tcp(struct s_env *env, struct s_params *params)
{
	printf("tcp to implement !");
	return FAILURE;
}

int	traceroute_udp(struct s_env *env, struct s_params *params)
{
	printf("udp to implement !");
	return FAILURE;
}

int	traceroute_udp_fixed_port(struct s_env *env, struct s_params *params)
{
	printf("udp fixed ports to implement !");
	return FAILURE;
}

int	traceroute(struct s_env *env, struct s_params *params)
{
	if (params->flags.icmp == 1)
		return traceroute_icmp(env, params);
	else if (params->flags.tcp == 1)
		return traceroute_tcp(env, params);
	else if (params->flags.udp == 1)
		return traceroute_udp_fixed_port(env, params);
	else
		return traceroute_udp(env, params);
}
int			main(int ac, char **av)
{
	struct s_env env;
	struct s_params params;
	int retval;

	retval = init_env(&env, av[0]);
	if (retval != SUCCESS)
		return retval;
	retval = init_params(&params);
	if (retval != SUCCESS)
		return retval;
	retval = parsing(ac, av, &params);
	if (retval != SUCCESS)
		return retval;
	retval = traceroute(&env, &params);
	free(params.host); // to delete for temp testing
	return 0;
}
