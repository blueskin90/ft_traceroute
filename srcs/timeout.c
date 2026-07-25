#include <string.h>
#include "ft_traceroute.h"

static void	get_max_timeout(struct timeval *send_time, float timeout_sec, struct timeval *timeout) {
	struct timeval to_add;
	int nsec;

	nsec = (int)timeout_sec;
	to_add.tv_sec = nsec;
	to_add.tv_usec = (timeout_sec - (float)nsec) * 1000000;
	add_timeval(send_time, &to_add, timeout);
}

static void	get_timeout_factor(struct timeval *send_time, struct timeval *rtt, float factor, struct timeval *result)
{
	struct timeval rtt_max;

	mult_timeval(rtt, factor, &rtt_max);
	add_timeval(send_time, &rtt_max, result);
}

// returns 1 if there is a here rtt available
static int	get_here_rtt(struct s_env *env, struct s_params *params, struct s_probe *probe, struct timeval *here_rtt)
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
static int	get_here_timeout(struct s_env *env, struct s_params *params, struct s_probe *probe, struct timeval *here_timeout)
{
	struct timeval here_rtt;

	if (get_here_rtt(env, params, probe, &here_rtt) != 1)
		return 0;
	get_timeout_factor(&probe->sent_time, &here_rtt, params->here_factor, here_timeout);
	return 1;
} 

// returns 1 if there is a near rtt available

static int	get_near_rtt(struct s_env *env, struct s_params *params, struct s_probe *probe, struct timeval *near_rtt)
{
	int idx_first_probe = probe->hop_num * params->probe_per_hop + params->probe_per_hop;
	struct s_probe *probe_ptr;
	int idx;

	for (idx = idx_first_probe; idx < env->probe_number; idx++) {
		probe_ptr = &(env->probes[idx]);
		if (probe_ptr->recv_answer) {
			sub_timeval(&probe_ptr->recv_time, &probe_ptr->sent_time, near_rtt);
			return 1;
		}
	}
	return 0;
}

// returns 1 if there is a near timeout available
static int	get_near_timeout(struct s_env *env, struct s_params *params, struct s_probe *probe, struct timeval *near_timeout)
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
