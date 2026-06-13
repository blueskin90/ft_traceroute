#include "ft_traceroute.h"
#include <string.h>
#include <sys/socket.h> 
#include <stdlib.h> // for the free ...
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>


static int	init_params(struct s_params *params)
{
	bzero(params, sizeof(struct s_params)); /* to init the flags */
	params->max_ttl = DEFAULT_MAX_TTL;
	params->first_ttl = DEFAULT_FIRST_TTL;
	params->probe_per_hop = DEFAULT_PROBE_PER_HOP;
	params->probe_burst = DEFAULT_PROBE_BURST;
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
	bzero(env, sizeof(*env));
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
	params->host = strdup("google.com"); // for temporary test without parsing
	//params->host = strdup("8.8.8.8"); // for temporary test without parsing
	// consider ./ft_traceroute -I 8.8.8.8
	// disallow any combination of flags for icmp / tcp / udp
	return SUCCESS;
}

int	init_probes(struct s_env *env, struct s_params *params)
{
	char hop_number = params->max_ttl - params->first_ttl + 1;
	int probe_number = hop_number * params->probe_per_hop;
	struct s_probe *probes = NULL;
	struct s_probe *probe = NULL;
	int seq = params->dest_port;
	int probe_index;
	char hop;

	probes = (struct s_probe*)malloc(sizeof(*probes) * probe_number);
	if (probes == NULL)
		return FAILURE;
	bzero(probes, sizeof(*probes) * probe_number);
	for (hop = 0; hop < hop_number; hop++) {
		for (probe_index = 0; probe_index < params->probe_per_hop; probe_index ++) {
			probe = &(probes[(hop * params->probe_per_hop) + probe_index]);
			probe->hop_num = hop;
			probe->seq = seq;
			probe->sent_ttl = hop + params->first_ttl;
			if (probe_index == 0)
				probe->first_in_hop = 1;
			else if (probe_index == params->probe_per_hop - 1)
				probe->last_in_hop = 1;
			seq++;
		}
	}
	env->probes = probes;
	env->hop_number = hop_number;
	env->probe_number = probe_number;
	return SUCCESS;
}

/*
** result = tv1 - tv2
**
** return values:
**	if tv1 < tv2: -1
**	if tv1 = tv2: 0
**	if tv1 > tv2: 1 
*/	
// sec -> ms == secs * 1000; ms -> us == ms * 1000; secs -> us == secs * 1 000 000
int	substract_timeval(struct timeval *tv1, struct timeval *tv2, struct timeval *result)
{
	result->tv_sec = tv1->tv_sec - tv2->tv_sec;
	result->tv_usec = tv1->tv_usec - tv2->tv_usec;

	if (result->tv_usec / 1000000) {
		int nsec = result->tv_usec / 1000000;
		
		result->tv_sec += nsec;
		result->tv_usec -= nsec * 1000000;
	}

	if (result->tv_sec < 0 || (result->tv_sec == 0 && result->tv_usec < 0))
		return -1;
	if (result->tv_sec > 0 || (result->tv_sec == 0 && result->tv_usec > 0))
		return 1;
	return 0;
}

void	add_timeval(struct timeval *tv1, struct timeval *tv2, struct timeval *result)
{
	result->tv_sec = tv1->tv_sec + tv2->tv_sec;
	result->tv_usec = tv1->tv_usec + tv2->tv_usec;
	if (result->tv_usec >= 1000000) {
		int nsec = result->tv_usec / 1000000;

		result->tv_sec += nsec;
		result->tv_usec -= nsec * 1000000;
	}
}

void	set_next_send(struct timeval *sent_time, struct timeval *next_send, float send_wait)
{
	struct timeval to_add;
	int nsec;

	if (send_wait > 10) {
		nsec = (int)(send_wait / 1000.0);
		to_add.tv_sec = nsec;
		to_add.tv_usec = (send_wait - (nsec * 1000)) * 1000;

	} // specifies ms case
	else {
		nsec = (int)send_wait;
		to_add.tv_sec = nsec;
		to_add.tv_usec = (send_wait - (float)nsec) * 1000000;
	}	
	add_timeval(sent_time, &to_add, next_send);
}

/* tv1 == now, tv2 == limit
** if now > limit return 0
** if now == limit return 0
** if now < limit return 1
*/
int	should_wait(struct timeval *limit, struct timeval *now)
{
	struct timeval result;

	if (substract_timeval(now, limit, &result) >= 0) {
		return 0; 
	}
	return 1;
}

int	send_probes(struct s_env *env, struct s_params *params)
{
	static int to_send = 0;
	static struct timeval next_send;	
	struct s_probe *probe;
	struct timeval now;
	int retval;

	if (env->probes_waiting > params->probe_burst)
		return SUCCESS;
	if (to_send != 0 && params->send_wait != 0) { // we send automatically with first send
		retval = gettimeofday(&now, NULL);
		if (retval != SUCCESS) {
			printf("couldnt get time of day\n");
			return FAILURE;
		}
		if (should_wait(&next_send, &now)) {
			return SUCCESS;
		}
	}
	probe = &env->probes[to_send];
	/*
	printf(" sending probe %d\n", to_send);
	printf(" sending probe for hop %hhd:", probe->hop_num); 
	if (probe->first_in_hop)
		printf("  It is the FIRST of its hop.\n");
	if (probe->last_in_hop)
		printf("  It is the LAST of its hop.\n");
	printf("  Its seq is: %d and its sent_ttl is %d\n", probe->seq, probe->sent_ttl);
	*/

	// build and send message
	// maybe set the sent_time when sent in this func

	retval = gettimeofday(&probe->sent_time, NULL);
	if (retval != SUCCESS) {
		printf("couldn't get time of day\n");
		return FAILURE;
	}

	probe->sent = 1;

	if (params->send_wait > 0) {
		set_next_send(&probe->sent_time, &next_send, params->send_wait);
	}
	to_send++;		
	env->probes_sent++;
	env->probes_waiting++;
	if (to_send >= env->probe_number)
		env->done_sending = 1;
	return SUCCESS;
}

int	receive_probes(struct s_env *env, struct s_params *params)
{
	(void)env;
	(void)params;
	return SUCCESS;
}

void	get_max_timeout(struct timeval *send_time, float timeout_sec, struct timeval *timeout) {
	struct timeval to_add;
	int nsec;

	nsec = (int)timeout_sec;
	to_add.tv_sec = nsec;
	to_add.tv_usec = (timeout_sec - (float)nsec) * 1000000;
	add_timeval(send_time, &to_add, timeout);
}

void	get_timeout_factor(struct timeval *send_time, struct timeval *rtt, float factor, struct timeval *result)
{
	(void)send_time;
	(void)rtt;
	(void)factor;
	(void)result;
}

void	get_smallest_timeout(struct s_probe *probe, int idx_probe, struct timeval *result, struct s_env *env, struct s_params *params)
{
	struct timeval max_timeout;
	struct timeval here_timeout;
	struct timeval near_timeout;

	struct timeval here_rtt;
	struct timeval near_rtt;
	// should init here_rtt and near_rtt;

	(void)idx_probe;
	(void)env;

	get_timeout_factor(&probe->sent_time, &here_rtt, params->here_factor, &here_timeout);
	get_timeout_factor(&probe->sent_time, &near_rtt, params->near_factor, &near_timeout);
	get_max_timeout(&probe->sent_time, params->max_timeout, &max_timeout);

	// should find which is the smallest return max for the moment
	memcpy(result, &max_timeout, sizeof(*result));
}

int	check_timeout_probe(struct s_probe *probe, int idx_probe, struct s_env *env, struct s_params *params) {
	// check if timeouted and set done + timeout in case
	(void)idx_probe;

	int retval;
	struct timeval now;
	struct timeval timeout;

	// replace with get_smallest_timeout later that return the smallest between neaf, here and max;
	get_smallest_timeout(probe, idx_probe, &timeout, env, params);
	retval = gettimeofday(&now, NULL);
	if (retval != SUCCESS) {
		printf("couldnt get time of day\n");
		return FAILURE;
	}
	if (should_wait(&timeout, &now)) {
		return SUCCESS;
	}
	env->probes_waiting--;
	env->probes_timeouted++;
	probe->done = 1;
	probe->timeout = 1;
	return SUCCESS;
}

int	timeout_probes(struct s_env *env, struct s_params *params)
{
	static int to_check = 0;
	int idx_check = to_check;
	struct s_probe *probe;
	int retval;

	while (idx_check < env->probe_number) {
		probe = &(env->probes[idx_check]);
		if (probe->sent == 0)
			return SUCCESS;

		retval = check_timeout_probe(probe, idx_check, env, params);
		if (retval != SUCCESS)
			return FAILURE;

		if (probe->done && to_check == idx_check) {
			to_check++; // increment for next check
			if (probe->last_in_hop && probe->is_host) {
				env->done_timeout = 1;
				return SUCCESS;
			}
		}
		idx_check++;
	}
	return SUCCESS;
}

int	print_probes(struct s_env *env, struct s_params *params)
{
	static int to_print = 0;
	struct s_probe *probe;
	int number_size;

	if (params->max_ttl < 10)
		number_size = 1;
	else if (params->max_ttl < 100)
		number_size = 2;
	else
		number_size = 3;

	while (to_print < env->probe_number) {
		probe = &(env->probes[to_print]);
		if (probe->done == 0)
			return SUCCESS;
		if (probe->first_in_hop)
			printf("%*hhd ", number_size, probe->sent_ttl);
		printf("* "); // case timeout, for test purpose
		// should print infos
		if (probe->last_in_hop) {
			printf("\n");
			if (probe->is_host)
				env->done_printing = 1;
		}
		to_print++;
	}
	env->done_printing = 1;
	return SUCCESS;
}

int	traceroute_icmp(struct s_env *env, struct s_params *params)
{
	(void)env;
	(void)params;
	int running = 1;
	int retval;
	
	while (running) {
		if (env->done_sending == 0) {
			retval = send_probes(env, params);
			if (retval != SUCCESS)
				return FAILURE;
		}
		if (env->done_receiving == 0) {
			retval = receive_probes(env, params);
			if (retval != SUCCESS)
				return FAILURE;
		}
		if (env->done_timeout == 0) {
			retval = timeout_probes(env, params);
			if (retval != SUCCESS)
				return FAILURE;
		}
		if (env->done_printing == 0)
			print_probes(env, params);
		if (env->done_printing == 1)
			running = 0;
	}
	return SUCCESS;
}

int	traceroute_tcp(struct s_env *env, struct s_params *params)
{
	(void)env;
	(void)params;
	printf("tcp to implement !");
	return FAILURE;
}

int	traceroute_udp(struct s_env *env, struct s_params *params)
{
	(void)env;
	(void)params;
	printf("udp to implement !");
	return FAILURE;
}

int	traceroute_udp_fixed_port(struct s_env *env, struct s_params *params)
{
	(void)env;
	(void)params;
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

#include <errno.h>
int	init_socket(struct s_env *env, struct s_params *params)
{
	(void)params;
	// icmp at the moment
	env->sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (env->sockfd < 0)
	{
		printf("Couldn't create the socket: %s\n", strerror(errno));
		return FAILURE;
	}
	return SUCCESS;
}

/*
void dump_addrinfo(struct addrinfo *res) {
    if (res == NULL) return;

    printf("--- addrinfo dump ---\n");
    printf("Flags:       %d\n", res->ai_flags);
    printf("Family:      %s (%d)\n", 
           (res->ai_family == AF_INET) ? "AF_INET" : 
           (res->ai_family == AF_INET6) ? "AF_INET6" : "AF_UNSPEC", res->ai_family);
    printf("Socktype:    %s (%d)\n", 
           (res->ai_socktype == SOCK_STREAM) ? "SOCK_STREAM" : 
           (res->ai_socktype == SOCK_DGRAM) ? "SOCK_DGRAM" : "OTHER", res->ai_socktype);
    printf("Protocol:    %d\n", res->ai_protocol);
    printf("Addrlen:     %u\n", (unsigned int)res->ai_addrlen);
    printf("Canonname:   %s\n", res->ai_canonname ? res->ai_canonname : "NULL");

    // Extracting IP address from ai_addr
    char host[NI_MAXHOST];
    if (res->ai_addr != NULL) {
        if (getnameinfo(res->ai_addr, res->ai_addrlen, host, sizeof(host), 
                        NULL, 0, NI_NUMERICHOST) == 0) {
            printf("IP Address:  %s\n", host);
        }
    }
    printf("---------------------\n");
}

void dump_sockaddr_in(const struct sockaddr_in *addr) {
    if (addr == NULL) return;

    char ip_str[INET_ADDRSTRLEN];
    
    // Convert the binary IP address to a human-readable string
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);
    
    // Convert the port from network byte order to host byte order
    uint16_t port = ntohs(addr->sin_port);

    printf("--- sockaddr_in dump ---\n");
    printf("Family:   AF_INET\n");
    printf("IP:       %s\n", ip_str);
    printf("Port:     %u\n", port);
    printf("------------------------\n");
}
*/

int	resolve_host(struct s_env *env, struct s_params *params)
{
	struct addrinfo hints;
	struct addrinfo *res = NULL;
	int retval;

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMP;
	// icmp for the moment
	retval = getaddrinfo(params->host, 0, &hints, &res);
	if (retval < 0) {
		fprintf(stderr, "%s: Couldn't resolve host %s: %s\n", env->prog, params->host, gai_strerror(retval));
		if (res)
			freeaddrinfo(res);
		return FAILURE;
	}
	if (res == NULL) {
		// no res ?
		return FAILURE;
	}
	/*
	for (struct addrinfo *tmp = res; tmp != NULL; tmp = tmp->ai_next) {
		dump_addrinfo(tmp);
	}
	*/
	env->dest_addr.sin_family = AF_INET;
	env->dest_addr.sin_port = 0;
	memcpy(&env->dest_addr.sin_addr, &((struct sockaddr_in*)res->ai_addr)->sin_addr, sizeof(env->dest_addr.sin_addr));
	freeaddrinfo(res);
	//dump_sockaddr_in(&env->dest_addr);
	return SUCCESS;
}

int	main(int ac, char **av)
{
	struct s_env env;
	struct s_params params;
	int retval;

	retval = init_env(&env, av[0]); // no malloc, might initialize a log struct / func ?
	if (retval != SUCCESS)
		return retval;
	retval = init_params(&params); // no malloc
	if (retval != SUCCESS)
		return retval;
	retval = parsing(ac, av, &params); // malloc because of testing purposes
	if (retval != SUCCESS)
		return retval;


	retval = resolve_host(&env, &params);
	if (retval != SUCCESS) {
		printf("resolve_host failed\n");
		free(params.host); // to delete for temp testing
		return retval;
	}
	retval = init_socket(&env, &params); // open socket
	if (retval != SUCCESS) {
		printf("init_socket failed\n");
		free(params.host); // to delete for temp testing
		return retval;
	}
	retval = init_probes(&env, &params); // malloc
	if (retval != SUCCESS) {
		printf("init_probes failed\n");
		close(env.sockfd);
		free(params.host); // to delete for temp testing
		return retval;
	}

	traceroute(&env, &params); // no malloc
	
	close(env.sockfd);
	free(env.probes);
	free(params.host); // to delete for temp testing
	return 0;
}
