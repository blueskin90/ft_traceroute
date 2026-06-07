#include "ft_traceroute.h"
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include "lib_arg_parsing.h"
#include <sys/socket.h>
#include <ctype.h>

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
	env->args.answer_timeout = DEFAULT_ANSWER_TIMEOUT_MS;
	env->ident = (uint16_t)getpid();
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

void	fill_header_icmp(struct s_env *env, char *buffer)
{
	struct icmp4_hdr *hdr = (struct icmp4_hdr *)buffer;

	hdr->msg_type = ECHO_REQUEST;
	hdr->ident = env->ident;
	hdr->sequence = env->seq;
}

void	fill_garbage(char *buffer, size_t bufsize, int offset)
{
	unsigned char val = 0x40 + offset;
	size_t i = 0;

	while (i < bufsize) {
		if (val > 0x7F)
			val = 0x40;
		buffer[i] = val;
		val++;
		i++;
	}
}

void	fill_buffer_timeval(struct s_env *env, char *buffer) // never call if no space
{
	struct timeval* time = (struct timeval*)buffer;

	gettimeofday(time, NULL);
	memcpy(&env->sent, time, sizeof(struct timeval));
}

void	fill_buffer(struct s_env *env, char *buffer, size_t bufsize)
{
	size_t offset = 0;
	if (bufsize > sizeof(struct timeval)) {
		fill_buffer_timeval(env, buffer);
		offset += sizeof(struct timeval);
	}
	fill_garbage(buffer + offset, bufsize - offset, offset);
}

int			compute_checksum(char *buffer, size_t buffsize)
{
	struct icmp4_hdr *hdr = (struct icmp4_hdr*)buffer;
	size_t buffsize_uint16 = buffsize / 2;
	uint16_t *buf = (uint16_t*)buffer;
	uint32_t checksum = 0;
	size_t i;

	hdr->checksum = 0;
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

void	fill_message_icmp(struct s_env *env, char *buffer, size_t bufsize)
{
	bzero(buffer, bufsize);
	fill_header_icmp(env, buffer);
	fill_buffer(env, buffer + ICMP_HDR_SIZE, bufsize - ICMP_HDR_SIZE);
	compute_checksum(buffer, bufsize);
}

void	print_first_line(struct s_env *env) {
	printf("traceroute to %s (%s), %hhu hops max. %u byte packet\n", env->args.dest, inet_ntoa(env->daddr.sin_addr), env->args.max_ttl, env->args.packet_len);
}

void	do_icmp_modifications(char *buffer, struct s_env *env) {
	struct icmp4_hdr *hdr = (struct icmp4_hdr *)buffer;

	hdr->sequence = htons(env->seq);
	if (env->args.packet_len - IPV4_HDR_SIZE - ICMP_HDR_SIZE > sizeof(struct timeval)) {
		fill_buffer_timeval(env, buffer + ICMP_HDR_SIZE);
	}
	compute_checksum(buffer, env->args.packet_len - IPV4_HDR_SIZE);
}

void	dump_message(char *buffer, int size) {
	struct icmp4_hdr *hdr = (struct icmp4_hdr*)(buffer + IPV4_HDR_SIZE);
	size -= IPV4_HDR_SIZE;

	printf("msg type: %hhd\n", hdr->msg_type);	
	printf("msg code: %hhd\n", hdr->code);	
	printf("msg checksum: %hx\n", hdr->checksum);	
	printf("msg ident: %hx\n", hdr->ident);	
	printf("msg sequence: %hx\n", hdr->sequence);	
	printf("\n");	
}

int	recv_msg(struct s_env *env) {
	char packet[MSG_SIZE];
	int size;	

	size = recv(env->sock, packet, MSG_SIZE, 0);
	while (size >= 0) {
		dump_message(packet, size);		
		size = recv(env->sock, packet, MSG_SIZE, 0);
	}
	return 1;
}

int	do_hop_icmp(char *buffer, struct s_env *env) {
	int retval = 0;
	int msg_to_send = env->args.probe_per_hop;
	int should_send = 1;

	setsockopt(env->sock, IPPROTO_IP, IP_TTL, &env->actual_hop, sizeof(env->actual_hop));

	while (msg_to_send > 0) {
		if (should_send) {
			do_icmp_modifications(buffer, env);
			retval = sendto(env->sock, buffer, env->args.packet_len - IPV4_HDR_SIZE, 0, (struct sockaddr*)&env->daddr, sizeof(env->daddr));
			if (retval < 0) {
				printf("%s: error : %s\n", env->progname, strerror(errno));
				return ERROR;
			}
			env->seq++;
			msg_to_send--;
			if (env->args.send_wait != 0) {
				should_send = 0;
				//send_wait_ms = env->args.send_wait;
			}
		}	
		// voir comment faire le wait entre les send + la reception en meme temps + le timeout
	}
	recv_msg(env);
	env->actual_hop++;
	return SUCCESS;
}

int	traceroute_icmp(struct s_env *env)
{
	char buffer[ICMP_HDR_SIZE + DATA_SIZE];

	if (open_icmp_socket(env) != SUCCESS)
		return ERROR;
	if (resolve_host_icmp(env) != SUCCESS)
		return ERROR;
	if (env->args.protocol.icmp.init_sequence == 0)
		env->seq = 1;
	else
		env->seq = env->args.protocol.icmp.init_sequence;
	if (env->args.packet_len < IPV4_HDR_SIZE + ICMP_HDR_SIZE)
		env->args.packet_len = IPV4_HDR_SIZE + ICMP_HDR_SIZE;
	env->actual_hop = env->args.start_ttl;

	print_first_line(env);
	fill_message_icmp(env, buffer, env->args.packet_len - IPV4_HDR_SIZE);
	do_hop_icmp(buffer, env);
	do_hop_icmp(buffer, env);
	return SUCCESS;
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
