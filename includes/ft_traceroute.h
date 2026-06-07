#ifndef FT_TRACEROUTE_H
# define FT_TRACEROUTE_H

#include <stdint.h>

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
};

enum e_status{
    SUCCESS = 0,
    FAILURE = 1
};

#define DEFAULT_MAX_TTL 30
#define DEFAULT_FIRST_TTL 1
#define DEFAULT_PROBE_PER_HOP 3
#define DEFAULT_PROBE_NUMBER 16
#define DEFAULT_STANDARD_UDP_DEST_PORT 33434
#define DEFAULT_CONSTANT_UDP_DEST_PORT 53
#define DEFAULT_CONSTANT_TCP_DEST_PORT 80
#define DEFAULT_ICMP_SEQ 1
#define DEFAULT_MAX_TIMEOUT 5.0
#define DEFAULT_HERE_FACTOR 3.0
#define DEFAULT_NEAR_FACTOR 10
#define DEFAULT_SEND_WAIT 0

#define DEFAULT_PACKET_LEN 40 + sizeof(struct iphdr)

struct s_flags {
	uint8_t no_host:1; /* -n do not map host to ip */
	uint8_t icmp:1; /* -I forced icmp */
	uint8_t tcp:1; /* -T forced tcp*/
	uint8_t udp:1; /* -U forced udp */
};

struct s_params {
	uint8_t max_ttl; /* -m max ttl, it will emit until this value if the target doesnt answer */
	uint8_t first_ttl; /* -f the first ttl value */
	uint8_t probe_per_hop; /* -q number of probes per hop */
	uint8_t probe_number; /* -N number of probes sent at the same time */
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
struct s_env
{
	char *prog;
	uint16_t pid;
};

# endif /* FT_TRACEROUTE_H */
