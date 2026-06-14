#include <sys/time.h> 

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
