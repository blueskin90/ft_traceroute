#ifndef FT_TRACEROUTE_H
# define FT_TRACEROUTE_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <stdio.h>
#include <strings.h>

#define MSG_SIZE 65507 + IPV4_HDR_SIZE + ICMP_HDR_SIZE
#define DATA_SIZE 65507
#define IPV4_HDR_SIZE 20
#define ICMP_HDR_SIZE 8

#define ICMP_TTL_EXCEEDED 11
#define ICMP_DEST_UNREACHABLE 3

#define IPV4_FORMAT "%hhu.%hhu.%hhu.%hhu"

#define IPV4_ARGUMENTS(x) x.addr_split[0], x.addr_split[1], x.addr_split[2], x.addr_split[3]

enum e_protocol {
	ICMP,
	UDP,
	TCP
}

#define DEFAULT_DATA_LEN 60
#define DEFAULT_MAX_TTL 30
#define DEFAULT_START_TTL 1
#define DEFAULT_PROBE_PER_HOP 3
#define DEFAULT_ANSWER_TIMEOUT 500;
#define DEFAULT_SEND_WAIT 0;
#define DEFAULT_PROTOCOL ICMP;

// handled flags: -m max ttl, -q nbre packet par ttl, -f first number of ttl, -w temps dattente dun retour (default 500 ms), -z temps d'attente entre l'envoi de chaque packet (default inconnu), maybe --mtu 
// optional packet_len uint64_t but max is DATA_SIZE

enum e_errorcode {
ERROR,
SUCCESS,
PARSING_ERROR,
INVALID_ARGUMENT,
INVALID_OPTION,
RESOLUTION_ERROR,
MUST_BE_HEX_ERROR,
SIZE_TOO_BIG,
INCORRECT_CHECKSUM,
INCORRECT_SIZE,
QUANTUM_PING,
MALLOC_ERROR,
SOCK_ERROR,
USAGE,
};

struct s_icmp {
	uint8_t msg_type;	
}

struct s_tcp {
	int sport;
	int dport;
}

struct s_udp {
	int sport;
	int dport;
}

union u_protocol {
	struct s_icmp icmp;
	struct s_tcp tcp;
	struct s_udp udp;
}

struct s_args {
	size_t data_size;
	uint8_t ttl;
	uint8_t start_ttl;
	uint8_t probe_per_hop;
	uint32_t send_wait;
	uint32_t packet_len;
	uint8_t protocol_type;
	union u_protocol protocol;
	char *dest;
}

struct s_env
{
	char *progname;
	struct sockaddr_in daddr;
	struct s_args args;
};

int args_parsing(struct s_env *env, int ac, char **av);

# endif /* FT_TRACEROUTE_H */
