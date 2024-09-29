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

#define ECHO_REQUEST 8
#define ECHO_REPLY 0

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

struct icmp4_hdr_notime {
	uint8_t	msg_type;
	uint8_t	code;
	uint16_t checksum;
	uint16_t ident;
	uint16_t sequence;
	char data[];
};

struct icmp4_hdr {
	uint8_t	msg_type;
	uint8_t	code;
	uint16_t checksum;
	uint16_t ident;
	uint16_t sequence;
	struct timeval time;
	char data[];
};

struct s_icmp {
	uint16_t init_sequence;
	uint16_t msg_type;	
};

struct s_tcp {
	uint16_t dport;
	uint16_t sport;
};

struct s_udp {
	uint16_t dport;
	uint16_t sport;
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
	struct timeval sent;
	uint16_t seq;
	uint16_t ident;
	char *progname;
	struct sockaddr_in daddr;
	struct s_args args;
	int sock;
};

int args_parsing(struct s_env *env, int ac, char **av);

# endif /* FT_TRACEROUTE_H */
