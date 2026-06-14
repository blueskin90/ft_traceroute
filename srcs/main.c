#include "ft_traceroute.h"
#include <string.h>
#include <sys/socket.h> 
#include <stdlib.h> // for the free ...
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>


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
	//params->send_wait = 1;
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
int	sub_timeval(struct timeval *tv1, struct timeval *tv2, struct timeval *result)
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

void	mult_timeval(struct timeval *tv, float factor, struct timeval *result)
{
	float result_sec = tv->tv_sec * factor;
	float result_usec = tv->tv_usec * factor;
	
	result->tv_sec = (int)result_sec;

	result_sec -= result->tv_sec;
	result_usec += result_sec * 1000000;

	if (result_usec >= 1000000) {
		int nbsec = result_usec / 1000000;

		result->tv_sec += nbsec;
		result_usec -= nbsec * 1000000;
	}
	result->tv_usec = result_usec;
}

/* returns -1 if tv1 is bigger (more recent) than tv2, 1 if tv2 is bigger than tv1, 0 otherwise */
int	cmp_timeval(struct timeval *tv1, struct timeval *tv2)
{
	if (tv1->tv_sec < tv2->tv_sec)
		return 1;
	if (tv1->tv_sec > tv2->tv_sec)
		return -1;
	if (tv1->tv_usec < tv2->tv_usec)
		return 1;
	if (tv1->tv_usec > tv2->tv_usec)
		return -1;
	return 0;
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
int	should_wait(struct timeval *now, struct timeval *limit)
{
	if (cmp_timeval(now, limit) <= 0)
		return 0; 
	return 1;
}
/*
	0x0040:  acd9 162e 97c1 82b7 01ec 309f 4041 4243  ..........0.@ABC
	0x0050:  4445 4647 4849 4a4b 4c4d 4e4f 5051 5253  DEFGHIJKLMNOPQRS
	0x0060:  5455 5657 5859 5a5b 5c5d 5e5f 6061 6263  TUVWXYZ[\]^_`abc
	0x0070:  6465 6667 6869 6a6b 6c6d 6e6f 7071 7273  defghijklmnopqrs
	0x0080:  7475 7677 7879 7a7b 7c7d 7e7f 4041 4243  tuvwxyz{|}~.@ABC
	0x0090:  4445 4647 4849 4a4b 4c4d 4e4f 5051 5253  DEFGHIJKLMNOPQRS
	0x00a0:  5455 5657 5859 5a5b 5c5d 5e5f 6061 6263  TUVWXYZ[\]^_`abc
	0x00b0:  6465 6667 6869 6a6b 6c6d 6e6f 7071 7273  defghijklmnopqrs
	0x00c0:  7475 7677 7879 7a7b 7c7d 7e7f 4041 4243  tuvwxyz{|}~.@ABC
	0x00d0:  4445 4647 4849 4a4b 4c4d 4e4f 5051 5253  DEFGHIJKLMNOPQRS
	0x00e0:  5455 5657 5859 5a5b 5c5d 5e5f 6061 6263  TUVWXYZ[\]^_`abc
	0x00f0:  6465 6667 6869 6a6b 6c6d 6e6f 7071 7273  defghijklmnopqrs
	0x0100:  7475 7677 7879 7a7b 7c7d 7e7f 4041 4243  tuvwxyz{|}~.@ABC
	0x0110:  4445 4647 4849 4a4b 4c4d 4e4f 5051 5253  DEFGHIJKLMNOPQRS
	0x0120:  5455 5657 5859 5a5b 5c5d 5e5f 6061 6263  TUVWXYZ[\]^_`abc
	0x0130:  6465 6667 6869 6a6b 6c6d 6e6f 7071 7273  defghijklmnopqrs

filling pattern
*/ 
// the parsing assure the size is enough

int	compute_checksum(char *buffer, size_t buffsize)
{
	struct icmp4_hdr *hdr = (struct icmp4_hdr*)buffer;
	size_t buffsize_uint16 = buffsize / 2;
	uint16_t *buf = (uint16_t*)buffer;
	uint32_t checksum = 0;
	size_t i;

	if (buffsize == 0)
		return 0;
	for(i = 0; i < buffsize_uint16; i++)
		checksum += buf[i];
	if (buffsize % 2)
		checksum += ((uint16_t)buffer[buffsize - 1]);
	// folding checksum to get an uint16_t back
	checksum = (checksum & 0xffff) + (checksum >> 16);
	hdr->checksum = ~checksum;
	return 1;
}

// to modify
void	fill_buffer(char *buffer, int bufsize) {
	int i = 0;

	while (i < bufsize) {
		buffer[i] = 'a';
		i++;
	}
}

void	fill_packet(struct s_probe *probe, char *buffer, int bufsize, struct s_env *env)
{
	struct icmp4_hdr *hdr = (struct icmp4_hdr *)buffer;

	bzero(buffer, bufsize);
	hdr->msg_type = ECHO_REQUEST;
	hdr->ident = env->pid;
	hdr->sequence = probe->seq;

	fill_buffer(buffer + sizeof(struct icmp4_hdr), bufsize - sizeof(struct icmp4_hdr));
	compute_checksum(buffer, bufsize);
}

int	send_probe_message(struct s_probe *probe, struct s_env *env, struct s_params *params)
{
	int retval;
	char buffer[MAX_PACKET_BUFFER];
	
	fill_packet(probe, buffer, params->packet_len - sizeof(struct iphdr), env);

	if (setsockopt(env->sockfd, IPPROTO_IP, IP_TTL, &probe->sent_ttl, sizeof(probe->sent_ttl)) < 0) {
		printf("setsockopt IP_TTL failed\n");
		return FAILURE;
	}
	retval = sendto(env->sockfd, buffer, params->packet_len - sizeof(struct iphdr), 0, (struct sockaddr*)&env->dest_addr, sizeof(env->dest_addr)); 
	if (retval < 0) {
		printf("couldn't send probe\n");
		return FAILURE;
	}
	retval = gettimeofday(&probe->sent_time, NULL);
	if (retval != SUCCESS) {
		printf("couldn't get time of day\n");
		return FAILURE;
	}
	probe->sent = 1;
	return SUCCESS;
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
		if (should_wait(&now, &next_send)) {
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

	retval = send_probe_message(probe, env, params);
	if (retval != SUCCESS)
		return FAILURE;

	if (params->send_wait > 0)
		set_next_send(&probe->sent_time, &next_send, params->send_wait);
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
	struct timeval rtt_max;

	mult_timeval(rtt, factor, &rtt_max);
	add_timeval(send_time, &rtt_max, result);
}

// returns 1 if there is a here rtt available
int	get_here_rtt(struct s_env *env, struct s_params *params, struct s_probe *probe, struct timeval *here_rtt)
{
	int idx_first_probe = probe->hop_num * params->probe_per_hop;
	int next_hop_first_probe = idx_first_probe + params->probe_per_hop;
	struct s_probe *probe_ptr;
	int idx;

	for (idx = idx_first_probe; idx < next_hop_first_probe && idx < env->probe_number; idx++) {
		probe_ptr = &(env->probes[idx]);
		if (probe_ptr->recv_answer) {
			sub_timeval(&probe_ptr->recv_time, &probe_ptr->sent_time, here_rtt);
			return 1;
		}
	}
	return 0;
}

// returns 1 if there is a here timeout available
int	get_here_timeout(struct s_env *env, struct s_params *params, struct s_probe *probe, struct timeval *here_timeout)
{
	struct timeval here_rtt;

	if (get_here_rtt(env, params, probe, &here_rtt) != 1)
		return 0;
	get_timeout_factor(&probe->sent_time, &here_rtt, params->here_factor, here_timeout);
	return 1;
} 

// returns 1 if there is a near rtt available

int	get_near_rtt(struct s_env *env, struct s_params *params, struct s_probe *probe, struct timeval *near_rtt)
{
	int idx_first_probe = probe->hop_num * params->probe_per_hop + params->probe_per_hop;
	int next_hop_first_probe = idx_first_probe + params->probe_per_hop;
	struct s_probe *probe_ptr;
	int idx;

	for (idx = idx_first_probe; idx < next_hop_first_probe && idx < env->probe_number; idx++) {
		probe_ptr = &(env->probes[idx]);
		if (probe_ptr->recv_answer) {
			sub_timeval(&probe_ptr->recv_time, &probe_ptr->sent_time, near_rtt);
			return 1;
		}
	}
	return 0;
}

// returns 1 if there is a neare timeout available
int	get_near_timeout(struct s_env *env, struct s_params *params, struct s_probe *probe, struct timeval *near_timeout)
{
	struct timeval near_rtt;

	if (get_near_rtt(env, params, probe, &near_rtt) != 1)
		return 0;
	get_timeout_factor(&probe->sent_time, &near_rtt, params->near_factor, near_timeout);
	return 1;
} 

void	get_smallest_timeout(struct s_probe *probe, struct timeval *result, struct s_env *env, struct s_params *params)
{
	int here_present;
	int near_present;
	struct timeval max_timeout;
	struct timeval here_timeout;
	struct timeval near_timeout;
	struct timeval *smallest;

	get_max_timeout(&probe->sent_time, params->max_timeout, &max_timeout);
	smallest = &max_timeout;

	here_present = get_here_timeout(env, params, probe, &here_timeout);
	near_present = get_near_timeout(env, params, probe, &near_timeout);
	if (here_present && cmp_timeval(smallest, &here_timeout) == -1)
		smallest = &here_timeout;
	if (near_present && cmp_timeval(smallest, &near_timeout) == -1)
		smallest = &near_timeout;
	
	memcpy(result, smallest, sizeof(*result));
}

int	check_timeout_probe(struct s_probe *probe, struct s_env *env, struct s_params *params) {
	int retval;
	struct timeval now;
	struct timeval timeout;

	get_smallest_timeout(probe, &timeout, env, params);
	retval = gettimeofday(&now, NULL);
	if (retval != SUCCESS) {
		printf("couldnt get time of day\n");
		return FAILURE;
	}
	if (should_wait(&now, &timeout)) {
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

		retval = check_timeout_probe(probe, env, params);
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
		if (probe->timeout)
			printf("* ");
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

void	print_first_line(struct s_env *env, struct s_params *params)
{
	char ip_str[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &(env->dest_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
	printf("traceroute to %s (%s), %hhd hops max, %d byte packets\n", params->host, ip_str, params->max_ttl, params->packet_len);
}

int	traceroute_icmp(struct s_env *env, struct s_params *params)
{
	int running = 1;
	int retval;
	
	print_first_line(env, params);
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

// need to do all of this becase "FCNTL FORBIDDEN GNEUGNEUGNEU so no non blocking reads.
int	set_sock_timeout(int sockfd) {
	struct timeval tv;

	tv.tv_sec = 0;           // 0 seconds
	tv.tv_usec = 1;     // 1 microseconds (0,001 millisecond)

	// Set the timeout option on the socket
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror("Error setting timeout");
		return FAILURE;
	}
	return SUCCESS;
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
	if (set_sock_timeout(env->sockfd) != SUCCESS)
		return FAILURE;
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
