#include "lib_arg_parsing_internal.h"
#include <stdio.h>
#include <string.h>
t_env env;

int			main(int ac, char **av)
{
	uint8_t first_ttl = 0;
	uint8_t max_ttl = 0;
	char mtu = 0;
	uint16_t port = 0;
	uint8_t queries = 0;
	char *host = NULL;
	uint32_t packetlen = 0;

	printf("init_result: %d\n", init_lib("./config/sample.ntmlp"));
	set_ptr(&first_ttl, "-f");
	set_ptr(&max_ttl, "-m");
	set_ptr(&port, "-p");
	set_ptr(&queries, "-q");
	set_string_ptr(&host, "host");
	set_ptr(&packetlen, "packetlen");
	set_bool_ptr_mask(&mtu, sizeof(char), 0x10, "--mtu");
	printf("parse_result: %d\n", parse(ac, av));
	printf("first_ttl = %hhu\n", first_ttl);
	printf("max_ttl = %hhu\n", max_ttl);
	printf("mtu = %hhd\n", mtu);
	printf("port = %hu\n", port);
	printf("queries = %hhu\n", queries);
	printf("host = %s\n", host);
	printf("packetlen = %u\n", packetlen);
	close_lib();
	return 0;
}

