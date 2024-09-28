#include "ft_traceroute.h"
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include "lib_arg_parsing.h"
#include <sys/socket.h>

void			dump_env(struct s_env *env)
{
	printf("progname: %s\n", env->progname);
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
	close_lib();
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
	env->args.answer_timeout = DEFAULT_ANSWER_TIMEOUT;
	return SUCCESS;
}

int	parsing(struct s_env *env, int ac, char **av)
{
	int retval;

	env->progname = av[0];
	retval = init_lib("config/config.ntmlp");
	if (retval != SUCCESS)
		return retval;
	set_ptr(&env->args.start_ttl, "-f");
	set_ptr(&env->args.max_ttl, "-m");
	set_ptr(&env->args.protocol.tcp.dport, "-p");
	set_ptr(&env->args.probe_per_hop, "-q");
	set_string_ptr(&env->args.dest, "host");
	set_ptr(&env->args.packet_len, "packetlen");
	set_bool_ptr_mask(&env->args.flags, sizeof(char), MTU_FLAG, "--mtu");
	retval = parse(ac, av);	
	return retval;
}


int	open_icmp_socket(struct s_env *env)
{
	env->sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	setsockopt(env->sock, IPPROTO_IP, IP_TTL, &env->args.max_ttl, sizeof(env->args.max_ttl));
	if (env->sock < 0)
	{
		fprintf(stderr, "%s: Couldn't create the socket: %s\n", env->progname, strerror(errno));
		return ERROR;
	}
	return SUCCESS;
}

int	resolve_host_icmp(struct s_env *env)
{
	struct addrinfo hints;
	struct addrinfo *res = NULL;
	int retval;

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMP;
	hints.ai_flags |= AI_CANONNAME;
	retval = getaddrinfo(env->args.dest, 0, &hints, &res);
	if (retval < 0) {
		fprintf(stderr, "%s: Couldn't resolve host %s: %s\n", env->progname, env->args.dest, gai_strerror(retval));
		if (res)
			freeaddrinfo(res);
		return ERROR;
	}
	env->daddr.sin_family = AF_INET;
	env->daddr.sin_port = 0;
	memcpy(&env->daddr.sin_addr, &((struct sockaddr_in*)res->ai_addr)->sin_addr, sizeof(env->daddr.sin_addr.s_addr));
	freeaddrinfo(res);
	return SUCCESS;

}

int	traceroute_icmp(struct s_env *env)
{
	char buffer[ICMP_HDR_SIZE + DATA_SIZE];

	if (open_icmp_socket(env) != SUCCESS)
		return ERROR;
	if (resolve_host_icmp(env) != SUCCESS)
		return ERROR;
	printf("traceroute to %s (%s), %hhu hops max. %u byte packet\n", env->args.dest, inet_ntoa(env->daddr.sin_addr), env->args.max_ttl, env->args.packet_len);
	(void)env;
	(void)buffer;
	return 0;
}

int	traceroute(struct s_env *env)
{
	if (env->args.protocol_type == ICMP)
		return traceroute_icmp(env);
	else
		fprintf(stderr, "nothing else than ICMP is handled at the moment\n");
	return ERROR;
}

int			main(int ac, char **av)
{
	struct s_env env;
	int retval;

	retval = init_env(&env);
	if (retval != SUCCESS)
		return retval;
	retval = parsing(&env, ac, av);
	if (retval != SUCCESS) {
		free_env(&env);
		return retval;
	}
//	dump_env(&env);
	traceroute(&env);
	free_env(&env);
	return 0;
}
