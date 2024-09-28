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

enum e_protocol {
	ICMP,
	UDP,
	TCP
};

#define DEFAULT_DATA_LEN 60
#define DEFAULT_MAX_TTL 30
#define DEFAULT_START_TTL 1
#define DEFAULT_PROBE_PER_HOP 3
#define DEFAULT_ANSWER_TIMEOUT 500;
#define DEFAULT_SEND_WAIT 50;
#define DEFAULT_PROTOCOL ICMP;

#define MTU_FLAG 0x1

// handled flags: -m max ttl, -q nbre packet par ttl, -f first number of ttl, -w temps dattente dun retour (default 500 ms), -z temps d'attente entre l'envoi de chaque packet (default inconnu), maybe --mtu 
// optional packet_len uint64_t but max is DATA_SIZE

struct s_icmp {
	uint8_t msg_type;	
};

struct s_tcp {
	int sport;
	int dport;
};

struct s_udp {
	int sport;
	int dport;
};

union u_protocol {
	struct s_icmp icmp;
	struct s_tcp tcp;
	struct s_udp udp;
};

struct s_args {
	uint8_t max_ttl;
	uint8_t start_ttl;
	uint8_t probe_per_hop;
	uint32_t send_wait;
	uint32_t answer_timeout;
	uint32_t packet_len;
	uint8_t protocol_type;
	uint8_t flags;
	union u_protocol protocol;
	char *dest;
};

struct s_env
{
	char *progname;
	struct sockaddr_in daddr;
	struct s_args args;
	int sock;
};

int args_parsing(struct s_env *env, int ac, char **av);

# endif /* FT_TRACEROUTE_H */
