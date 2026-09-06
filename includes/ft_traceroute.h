#ifndef FT_TRACEROUTE_H
# define FT_TRACEROUTE_H

#include <stdint.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define ECHO_REQUEST 8
#define ECHO_REPLY 0

#define PROTOCOL_ICMP 1
#define PROTOCOL_UDP 17

#define ICMP_TTL_EXCEEDED 11
#define ICMP_DEST_UNREACHABLE 3

struct icmp4_hdr {
	uint8_t	msg_type;
	uint8_t	code;
	uint16_t checksum;
	uint16_t ident;
	uint16_t sequence;
	char data[];
} __attribute__((packed));

struct udp_hdr {
	uint16_t source;
	uint16_t dest;
	uint16_t len;
	uint16_t checksum;
} __attribute__((packed));

struct iphdr {
    	#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
		uint8_t ihl:4;
		uint8_t version:4;
	#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
		uint8_t version:4;
		uint8_t ihl:4;
    	#else
		#error  "Couldn't define endianness"
    	#endif
	uint8_t tos;
	uint16_t tot_len;
	uint16_t id;
	uint16_t frag_off;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t check;
	uint32_t saddr;
	uint32_t daddr;
} __attribute__((packed));

#define DEFAULT_MAX_TTL 30
#define DEFAULT_FIRST_TTL 1
#define DEFAULT_PROBE_PER_HOP 3
#define DEFAULT_PROBE_BURST 16
#define DEFAULT_STANDARD_UDP_DEST_PORT 33434
#define DEFAULT_CONSTANT_UDP_DEST_PORT 53
#define DEFAULT_CONSTANT_TCP_DEST_PORT 80
#define DEFAULT_ICMP_SEQ 1
#define DEFAULT_MAX_TIMEOUT 5.0
#define DEFAULT_HERE_FACTOR 3.0
#define DEFAULT_NEAR_FACTOR 10
#define DEFAULT_SEND_WAIT 0

#define DEFAULT_PACKET_LEN 40 + sizeof(struct iphdr)

#define MAX_PACKET_BUFFER 256
// to change

/*
	min packet size = 28

#define MAX_HOPS	255
#define MAX_PROBES	10
#define MAX_GATEWAYS_4	8
#define MAX_GATEWAYS_6	127
#define DEF_HOPS	30
#define DEF_SIM_PROBES	16	
#define DEF_NUM_PROBES	3
#define DEF_WAIT_SECS	5.0
#define DEF_HERE_FACTOR	3
#define DEF_NEAR_FACTOR	10
#ifndef DEF_WAIT_PREC
#define DEF_WAIT_PREC	0.001	// +1 ms  to avoid precision issues
#endif
#define DEF_SEND_SECS	0
#define DEF_DATA_LEN	40	//  all but IP header...
#define MAX_PACKET_LEN	65000


*/

struct s_flags {
	uint8_t back:1; /* --back estimate the return ttl based on 64, 128 or 255 if different from sent ttl */
	uint8_t no_host:1; /* -n do not map host to ip */
	uint8_t icmp:1; /* -I forced icmp */
	uint8_t tcp:1; /* -T forced tcp*/
	uint8_t udp:1; /* -U forced udp */
};

struct s_params {
	uint8_t max_ttl; /* -m max ttl, it will emit until this value if the target doesnt answer */
	uint8_t first_ttl; /* -f the first ttl value */
	uint8_t probe_per_hop; /* -q number of probes per hop */
	uint8_t probe_burst; /* -N number of probes sent at the same time */
	uint16_t dest_port;
	/*
	** -p for ICMP, initial sequence number incremented with each probe
	** for standard UDP the dest port used for the first probe (increment each probe)
	** for specific UDP (-U) the dest port constant
	** for TCP the constant dest port
	*/
	float max_timeout; /* part of -w max, in seconds in any case*/
	float here_factor; /* part of -w, if already responded, timeout for same node*/
	float near_factor; /* part of -w, if next already responded, timeout this node */
	float send_wait; /* -z if > 10, in ms, otherwise in s */
	struct s_flags flags;

	char *host; /* mandatory argument */
	uint32_t packet_len; /* optional argument at the end, count the ip header in */
};
// also handle -V / --version and --help

struct s_probe {
	/* init values */
	uint8_t hop_num;
	char first_in_hop:1;
	char last_in_hop:1;
	int seq;
	uint8_t sent_ttl;
	/* emission values */
	struct timeval sent_time;
	char sent:1;
	/* reception value */
	char done:1;
	char recv_answer:1;
	char timeout:1;
	char is_host:1;
	char is_printed:1;
	uint8_t recv_ttl;
	struct timeval recv_time;
	uint32_t recv_addr; // address who sent us the answer
/*
struct timeval {
	time_t      tv_sec;
	suseconds_t tv_usec;
};
*/
};

struct s_env
{
	char hop_number;
	int probe_number;

	// for debug
	int probes_sent;
	int probes_waiting;
	int probes_received;
	int probes_timeouted; 

	uint8_t done_sending:1;
	uint8_t done_timeout:1;
	uint8_t done_receiving:1;
	uint8_t done_printing:1;
	uint8_t found_host:1;
	struct s_probe *probes;
	int sockfd;
	int sendfd;
	char *prog;
	uint16_t pid;
	struct sockaddr_in dest_addr;
};

/* Timeval functions */


int	sub_timeval(struct timeval *tv1, struct timeval *tv2, struct timeval *result);
void	add_timeval(struct timeval *tv1, struct timeval *tv2, struct timeval *result);
void	mult_timeval(struct timeval *tv, float factor, struct timeval *result);
int	cmp_timeval(struct timeval *tv1, struct timeval *tv2);

/* Timeout functions */

void	get_smallest_timeout(struct s_probe *probe, struct timeval *result, struct s_env *env, struct s_params *params);
# endif /* FT_TRACEROUTE_H */
