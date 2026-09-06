#include "ft_traceroute.h"
#include <string.h>
#include <sys/socket.h> 
#include <stdlib.h> // for the free ...
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include "lib_arg_parsing.h"


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
	int retval;

	retval = init_lib("config/config.ntmlp");
	if (retval != SUCCESS)
		return retval;

	set_bool_ptr_mask(&params->flags, sizeof(params->flags), 0b1, "--back");
	set_bool_ptr_mask(&params->flags, sizeof(params->flags), 0b10, "-n");
	set_bool_ptr_mask(&params->flags, sizeof(params->flags), 0b100, "-I");
	set_bool_ptr_mask(&params->flags, sizeof(params->flags), 0b1000, "-T");
	set_bool_ptr_mask(&params->flags, sizeof(params->flags), 0b10000, "-U");
	set_ptr(&params->first_ttl, "-f");
	set_ptr(&params->max_ttl, "-m");
	set_ptr(&params->probe_burst, "-N"); // to change
	set_ptr(&params->dest_port, "-p");
	set_ptr(&params->probe_per_hop, "-q");
	set_string_ptr(&params->host, "host");
	set_ptr(&params->packet_len, "packetlen");
	retval = parse(ac, av);
	if (retval != SUCCESS) {
		close_lib();
		return retval;
	}

	if (params->flags.icmp + params->flags.udp + params->flags.tcp > 1) {
		printf("can't enable a combination of -I -T -U\n");
		close_lib();
		return -1;
	}
	if (params->dest_port == 0) {
		if (params->flags.icmp == 1)
			params->dest_port = DEFAULT_ICMP_SEQ;
		else if (params->flags.tcp == 1)
			params->dest_port = DEFAULT_CONSTANT_TCP_DEST_PORT;
		else if (params->flags.udp == 1)
			params->dest_port = DEFAULT_CONSTANT_UDP_DEST_PORT;
		else
			params->dest_port = DEFAULT_STANDARD_UDP_DEST_PORT;
	}
	close_lib();
	return retval;
	// missing -w ! but mor or less easy to implement
	// it still mallocs !
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
			if (probe_index == params->probe_per_hop - 1)
				probe->last_in_hop = 1;
			seq++;
		}
	}
	env->probes = probes;
	env->hop_number = hop_number;
	env->probe_number = probe_number;
	return SUCCESS;
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
	} // specifies seconds case
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

void	fill_icmp_packet(struct s_probe *probe, char *buffer, int bufsize, struct s_env *env)
{
	struct icmp4_hdr *hdr = (struct icmp4_hdr *)buffer;

	bzero(buffer, bufsize);
	hdr->msg_type = ECHO_REQUEST;
	hdr->ident = htons(env->pid);
	hdr->sequence = htons(probe->seq);

	fill_buffer(buffer + sizeof(struct icmp4_hdr), bufsize - sizeof(struct icmp4_hdr));
	compute_checksum(buffer, bufsize);
}

void	fill_udp_packet(struct s_probe *probe, char *buffer, int bufsize,
		struct s_env *env, struct s_params *params)
{
	struct udp_hdr *hdr = (struct udp_hdr *)buffer;
	uint16_t probe_index;

	bzero(buffer, bufsize);
	probe_index = probe->seq - params->dest_port;
	if (params->flags.udp) {
		hdr->source = htons(49152 + probe_index);
		hdr->dest = htons(params->dest_port);
	}
	else {
		hdr->source = htons(env->pid);
		hdr->dest = htons(probe->seq);
	}
	hdr->len = htons(bufsize);
	fill_buffer(buffer + sizeof(*hdr), bufsize - sizeof(*hdr));
}

int	send_probe_message(struct s_probe *probe, struct s_env *env, struct s_params *params)
{
	int retval;
	char buffer[MAX_PACKET_BUFFER];
	
	if (params->flags.icmp)
		fill_icmp_packet(probe, buffer, params->packet_len - sizeof(struct iphdr), env);
	else
		fill_udp_packet(probe, buffer, params->packet_len - sizeof(struct iphdr),
			env, params);

	if (setsockopt(env->sendfd, IPPROTO_IP, IP_TTL, &probe->sent_ttl, sizeof(probe->sent_ttl)) < 0) {
		printf("setsockopt IP_TTL failed\n");
		return FAILURE;
	}
	retval = sendto(env->sendfd, buffer,
		params->packet_len - sizeof(struct iphdr), 0,
		(struct sockaddr*)&env->dest_addr, sizeof(env->dest_addr));
	if (retval < 0) {
		printf("couldn't send probe\n");
		return FAILURE;
	}
	retval = gettimeofday(&probe->sent_time, NULL);
	if (retval < 0) {
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

	if (env->probes_waiting >= params->probe_burst)
		return SUCCESS;
	if (to_send != 0 && params->send_wait != 0) { // we send automatically with first send
		retval = gettimeofday(&now, NULL);
		if (retval < 0) {
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

int verify_checksum(char* buffer, size_t buffsize)
{
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
	checksum = (checksum & 0xffff) + (checksum >> 16);
	return (checksum == 0xffff);
}

#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>

void dump_ip_header(const struct iphdr *ip) {
	char saddr_str[INET_ADDRSTRLEN];
	char daddr_str[INET_ADDRSTRLEN];

	// Convert network addresses to human-readable strings
	inet_ntop(AF_INET, &(ip->saddr), saddr_str, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(ip->daddr), daddr_str, INET_ADDRSTRLEN);

	printf("--- IPv4 Header Dump ---\n");
	printf("Version      : %u\n", ip->version);
	printf("IHL          : %u (Words: %u, Bytes: %u)\n", ip->ihl, ip->ihl, ip->ihl * 4);
	printf("TOS          : 0x%02x\n", ip->tos);
	printf("Total Length : %u\n", ntohs(ip->tot_len));
	printf("ID           : %u\n", ntohs(ip->id));
	printf("Frag Offset  : %u\n", ntohs(ip->frag_off));
	printf("TTL          : %u\n", ip->ttl);
	printf("Protocol     : %u\n", ip->protocol);
	printf("Checksum     : 0x%04x\n", ntohs(ip->check));
	printf("Source Addr  : %s\n", saddr_str);
	printf("Dest Addr    : %s\n", daddr_str);
	printf("------------------------\n");
}

void dump_icmp_header(const struct icmp4_hdr *icmp) {
	    printf("--- ICMP Header Dump ---\n");
	    printf("Type     : %u\n", icmp->msg_type);
	    printf("Code     : %u\n", icmp->code);
	    printf("Checksum : 0x%04x\n", ntohs(icmp->checksum));
	    printf("ID       : %u\n", ntohs(icmp->ident));
	    printf("Sequence : %u\n", ntohs(icmp->sequence));
	    printf("------------------------\n");
}

struct s_probe* find_probe(struct icmp4_hdr *hdr, struct s_env *env, struct s_params *params) {
	uint16_t sequence = ntohs(hdr->sequence);
	int idx = sequence - params->dest_port;
	
	if (idx < 0 || idx >= env->probe_number)
		return NULL;
	return &(env->probes[idx]);
}

struct s_probe* find_udp_probe(struct udp_hdr *hdr, struct s_env *env,
		struct s_params *params)
{
	int idx;

	if (params->flags.udp)
		idx = ntohs(hdr->source) - 49152;
	else
		idx = ntohs(hdr->dest) - params->dest_port;
	if (idx < 0 || idx >= env->probe_number)
		return NULL;
	return &env->probes[idx];
}

void	fill_probe(struct s_probe *probe, struct iphdr *iphdr, struct timeval *recv_time, int is_host)
{
	probe->done = 1;
	probe->recv_answer = 1;
	probe->is_host = is_host & 1;
	probe->recv_ttl = iphdr->ttl;
	probe->recv_addr = iphdr->saddr;
	memcpy(&probe->recv_time, recv_time, sizeof(struct timeval));
}

void	dump_probe(struct s_probe *probe) {
	char saddr_str[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &(probe->recv_addr), saddr_str, INET_ADDRSTRLEN);

	printf("--probe dump--\n");
	printf("Source Addr  : %s\n", saddr_str);
	printf("hop num: %d\n", probe->hop_num);
	printf("sent ttl %hhd\n", probe->sent_ttl);
	printf("------------------------\n");

}

int	parse_response(struct s_env *env, struct s_params *params, char *packet, int packetlen)
{
	struct iphdr *iphdr = (struct iphdr*)packet;
	struct icmp4_hdr *icmphdr;
	struct iphdr *quoted_ip;
	void *quoted_transport;
	struct timeval recv_time;
	struct s_probe *probe = NULL;
	int outer_ihl;
	int quoted_ihl;
	int is_host = 0;

	if (packetlen < (int)(sizeof(*iphdr) + sizeof(*icmphdr))
		|| iphdr->protocol != PROTOCOL_ICMP)
		return SUCCESS;
	outer_ihl = iphdr->ihl * 4;
	if (outer_ihl < (int)sizeof(*iphdr)
		|| packetlen < outer_ihl + (int)sizeof(*icmphdr))
		return SUCCESS;
	icmphdr = (struct icmp4_hdr *)(packet + outer_ihl);
	if (!verify_checksum((char*)icmphdr, packetlen - outer_ihl))
		return SUCCESS;
	gettimeofday(&recv_time, NULL);
	if (params->flags.icmp && icmphdr->msg_type == ECHO_REPLY) {
		if (ntohs(icmphdr->ident) != env->pid)
			return SUCCESS;
		probe = find_probe(icmphdr, env, params);
	}
	else if (icmphdr->msg_type == ICMP_TTL_EXCEEDED
		|| icmphdr->msg_type == ICMP_DEST_UNREACHABLE) {
		if (packetlen < outer_ihl + (int)sizeof(*icmphdr)
			+ (int)sizeof(*quoted_ip))
			return SUCCESS;
		quoted_ip = (struct iphdr *)((char *)icmphdr + sizeof(*icmphdr));
		quoted_ihl = quoted_ip->ihl * 4;
		if (quoted_ihl < (int)sizeof(*quoted_ip)
			|| packetlen < outer_ihl + (int)sizeof(*icmphdr) + quoted_ihl + 8)
			return SUCCESS;
		quoted_transport = (char *)quoted_ip + quoted_ihl;
		if (params->flags.icmp && quoted_ip->protocol == PROTOCOL_ICMP) {
			if (ntohs(((struct icmp4_hdr *)quoted_transport)->ident) != env->pid)
				return SUCCESS;
			probe = find_probe(quoted_transport, env, params);
		}
		else if (!params->flags.icmp && quoted_ip->protocol == PROTOCOL_UDP)
			probe = find_udp_probe(quoted_transport, env, params);
		is_host = (!params->flags.icmp
			&& icmphdr->msg_type == ICMP_DEST_UNREACHABLE
			&& icmphdr->code == 3);
	}
	if (probe == NULL || probe->done)
		return SUCCESS;
	if (params->flags.icmp && icmphdr->msg_type == ECHO_REPLY)
		is_host = 1;
	fill_probe(probe, iphdr, &recv_time, is_host);
	if (is_host)
		env->found_host = 1;
	env->probes_waiting--;
	env->probes_received++;
	if (probe->last_in_hop && probe->is_host) {
		env->done_sending = 1;
		env->done_receiving = 1;
	}
	return SUCCESS;
}

int	receive_probes(struct s_env *env, struct s_params *params)
{
	char packet[MAX_PACKET_BUFFER];
	int retval;

	bzero(&packet, MAX_PACKET_BUFFER);
	retval = recvfrom(env->sockfd, packet, MAX_PACKET_BUFFER, MSG_DONTWAIT, NULL, NULL);
	while (retval > 0) {
		parse_response(env, params, packet, retval);
		retval = recvfrom(env->sockfd, packet, MAX_PACKET_BUFFER, MSG_DONTWAIT, NULL, NULL);
	}
	if (retval < 0) { if (errno == EAGAIN || errno == EWOULDBLOCK)
			return SUCCESS;
		printf("error when receiving\n");
		return FAILURE;
	}
	return SUCCESS;
}

int	check_timeout_probe(struct s_probe *probe, struct s_env *env, struct s_params *params) {
	int retval;
	struct timeval now;
	struct timeval timeout;

	get_smallest_timeout(probe, &timeout, env, params);
	retval = gettimeofday(&now, NULL);
	if (retval < 0) {
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
	//printf("timeouted a probe !\n");
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
		if (probe->done == 0) {
			retval = check_timeout_probe(probe, env, params);
			if (retval != SUCCESS)
				return FAILURE;
		}
		if (probe->done && to_check == idx_check) {
			to_check++; // increment for next check
			if (probe->last_in_hop && probe->is_host) {
				env->done_timeout = 1;
				env->done_receiving = 1;
				return SUCCESS;
			}
		}
		idx_check++;
	}
	return SUCCESS;
}


void	print_probe_rtt(struct s_probe *probe)
{
	struct timeval rtt;

	sub_timeval(&probe->recv_time, &probe->sent_time, &rtt);
	printf("  %.3f ms", (float)rtt.tv_sec * 1000 + (float)rtt.tv_usec / 1000);
}

int	is_first_probe_address(struct s_probe *probe, int probe_idx, struct s_env *env, struct s_params *params)
{
	int first_probe_idx = probe->hop_num * params->probe_per_hop;
	int idx;

	if (probe->first_in_hop)
		return 1;
	// issue in this function
	idx = first_probe_idx;
	while (idx < probe_idx) {
		if (idx == probe_idx || env->probes[idx].done == 0) {
			idx++;
			continue;
		}
		if (memcmp(&(env->probes[idx].recv_addr), &(probe->recv_addr), sizeof(probe->recv_addr)) == 0)
			return 0; 
		idx++;
	}
	return 1;
}

void	print_probe_address(struct s_probe *probe, struct s_env *env, struct s_params *params)
{
	char saddr_str[INET_ADDRSTRLEN];

	struct sockaddr_in sa;
	char hostname[NI_MAXHOST];

	inet_ntop(AF_INET, &(probe->recv_addr), saddr_str, INET_ADDRSTRLEN);
	if (params->flags.no_host) {
    		printf(" %s ", saddr_str);
		return;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	inet_pton(AF_INET, saddr_str, &sa.sin_addr);

	int result = getnameinfo((struct sockaddr *)&sa, sizeof(sa), 
				 hostname, sizeof(hostname), 
				 NULL, 0, NI_NAMEREQD);

	if (result == 0) {
	    printf(" %s (%s)", hostname, saddr_str);
	} else {
	    printf(" %s (%s)", saddr_str, saddr_str);
	}
	(void)env;
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
		if (probe->first_in_hop) {
			printf("%*hhd ", number_size, probe->sent_ttl);
		}
		if (probe->timeout)
			printf(" *");
		else {
			if (is_first_probe_address(probe, to_print, env, params))
				print_probe_address(probe, env, params);
			print_probe_rtt(probe);
		}
		if (probe->last_in_hop) {
			printf("\n");
			if (probe->is_host) {
				env->done_printing = 1;
				return SUCCESS;
			}
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

int	traceroute_loop(struct s_env *env, struct s_params *params)
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
			// when uncommenting this block, infinite loop
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

int	traceroute(struct s_env *env, struct s_params *params)
{
	if (params->flags.tcp == 1)
		return traceroute_tcp(env, params);
	return traceroute_loop(env, params);
}

// need to do all of this becase "FCNTL FORBIDDEN GNEUGNEUGNEU so no non blocking reads.
// might need to delete it tho maybe useless
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
	env->sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (env->sockfd < 0)
	{
		printf("Couldn't create the socket: %s\n", strerror(errno));
		return FAILURE;
	}
	if (set_sock_timeout(env->sockfd) != SUCCESS)
		return FAILURE;
	if (params->flags.icmp)
		env->sendfd = env->sockfd;
	else {
		env->sendfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
		if (env->sendfd < 0) {
			printf("Couldn't create UDP socket: %s\n", strerror(errno));
			close(env->sockfd);
			return FAILURE;
		}
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

*/
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
	//dump_sockaddr_in(&env->dest_addr);
	return SUCCESS;
}
#include <stdio.h>

void dump_env(const struct s_env *env) {
    if (!env) return;

    printf("--- Application State (s_env) ---\n");
    printf("Hop Number      : %d\n", env->hop_number);
    printf("Probe Number    : %d\n", env->probe_number);
    
    printf("\n[Statistics]\n");
    printf("Sent            : %d\n", env->probes_sent);
    printf("Waiting         : %d\n", env->probes_waiting);
    printf("Received        : %d\n", env->probes_received);
    printf("Timeouted       : %d\n", env->probes_timeouted);

    printf("\n[Flags]\n");
    printf("Done Sending    : %d\n", env->done_sending);
    printf("Done Timeout    : %d\n", env->done_timeout);
    printf("Done Receiving  : %d\n", env->done_receiving);
    printf("Done Printing   : %d\n", env->done_printing);
    printf("Found Host      : %d\n", env->found_host);

    printf("\n[Configuration]\n");
    printf("Socket FD       : %d\n", env->sockfd);
    printf("Program Name    : %s\n", env->prog ? env->prog : "NULL");
    printf("PID             : %u\n", env->pid);

    // Call the helper function from previous step
    dump_sockaddr_in(&env->dest_addr);
    
    printf("---------------------------------\n");
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
		return retval;
	}
	retval = init_socket(&env, &params); // open socket
	if (retval != SUCCESS) {
		printf("init_socket failed\n");
		return retval;
	}
	retval = init_probes(&env, &params); // malloc
	if (retval != SUCCESS) {
		printf("init_probes failed\n");
		close(env.sockfd);
		return retval;
	}

	//dump_env(&env);
	traceroute(&env, &params); // no malloc
	
	if (env.sendfd != env.sockfd)
		close(env.sendfd);
	close(env.sockfd);
	free(env.probes);
	return 0;
}
