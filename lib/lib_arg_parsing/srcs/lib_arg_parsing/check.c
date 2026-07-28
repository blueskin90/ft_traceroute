#include "lib_arg_parsing_internal.h"
#include <limits.h> 
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

int unsigned_check(char *val, struct s_flag *flag)
{
	uint64_t value;
	char *end = NULL;

	value = strtoull(val, &end, 0);
	if (value == 0 && end == val)
		return invalid_argument(flag, val);
	if (end && *end != 0)
		return invalid_argument(flag, val);
	if (value < flag->complementary.unsigned_integer.min ||
		value > flag->complementary.unsigned_integer.max)
			return incorrect_value_unsigned(flag, val);
    return SUCCESS;
}

int signed_check(char *val, struct s_flag *flag)
{
	int64_t value;
	char *end = NULL;

	value = strtoll(val, &end, 0);
	if (value == 0 && end == val)
		return invalid_argument(flag, val);
	if (end && *end != 0)
		return invalid_argument(flag, val);
	if (value < flag->complementary.integer.min ||
		value > flag->complementary.integer.max)
			return incorrect_value_signed(flag, val);
    return SUCCESS;
}

int float_check(char *val, struct s_flag *flag)
{
	char *end = NULL;
	float value;

	errno = 0;
	value = strtof(val, &end);
	if (end == val || *end != 0 || errno == ERANGE || !isfinite(value))
		return invalid_argument(flag, val);
	if (value < flag->complementary.float_values.min
		|| value > flag->complementary.float_values.max)
		return invalid_argument(flag, val);
	return SUCCESS;
}

int string_check(char *val, struct s_flag *flag)
{
    (void)val;
    (void)flag;

    return SUCCESS;
}

int dummy_check(char *val, struct s_flag *flag)
{
    (void)val;
    (void)flag;

    return SUCCESS;
}

int char_check(char *val, struct s_flag *flag)
{
	char value;
	if (strlen(val) > 1)
		return invalid_argument(flag, val);
	value = *val;
	if (value < flag->complementary.integer.min ||
		value > flag->complementary.integer.max)
			return incorrect_value_signed(flag, val);
    return SUCCESS;
}

int unsigned_parse(char *val, struct s_flag *flag)
{
	uint64_t value;

	value = strtoull(val, NULL, 0);
	switch (flag->type) {
		case UINT8_T: {
			uint8_t *ptr = (uint8_t*)flag->data;
			*ptr = (uint8_t)value;
			break;
		}
		case UINT16_T: {
			uint16_t *ptr = (uint16_t*)flag->data;
			*ptr = (uint16_t)value;
			break;
		}
		case UINT32_T: {
			uint32_t *ptr = (uint32_t*)flag->data;
			*ptr = (uint32_t)value;
			break;
		}
		case UINT64_T: {
			uint64_t *ptr = (uint64_t*)flag->data;
			*ptr = value;
			break;
		}
		default: value = 0;
	}
    return SUCCESS;
}

int signed_parse(char *val, struct s_flag *flag)
{
	int64_t value;

	value = strtoll(val, NULL, 0);
	switch (flag->type) {
		case INT8_T: {
			int8_t *ptr = (int8_t*)flag->data;
			*ptr = (int8_t)value;
			break;
		}
		case INT16_T: {
			int16_t *ptr = (int16_t*)flag->data;
			*ptr = (int16_t)value;
			break;
		}
		case INT32_T: {
			int32_t *ptr = (int32_t*)flag->data;
			*ptr = (int32_t)value;
			break;
		}
		case INT64_T: {
			int64_t *ptr = (int64_t*)flag->data;
			*ptr = value;
			break;
		}
		default: return ERROR;
	}
    return SUCCESS;
}

int float_parse(char *val, struct s_flag *flag)
{
	float *ptr = (float *)flag->data;

	*ptr = strtof(val, NULL);
	return SUCCESS;
}

int string_parse(char *val, struct s_flag *flag)
{
	char **ptr = (char**)flag->data;
	*ptr = val;
    return SUCCESS;
}

int char_parse(char *val, struct s_flag *flag)
{
	char *ptr = (char*)flag->data;

	*ptr = *val;
    return SUCCESS;
}

int bool_parse(char *dummy, struct s_flag *flag)
{
    (void)dummy;
    if (flag->complementary.bool_values.ptr_size == sizeof(uint8_t)) {
		uint8_t *ptr = (uint8_t*)flag->data;
		(*ptr) |= (uint8_t)flag->complementary.bool_values.mask;
    }
	else if (flag->complementary.bool_values.ptr_size == sizeof(uint16_t)) {
		uint16_t *ptr = (uint16_t*)flag->data;
		(*ptr) |= (uint16_t)flag->complementary.bool_values.mask;
    }
	else if (flag->complementary.bool_values.ptr_size == sizeof(uint32_t)) {
		uint32_t *ptr = (uint32_t*)flag->data;
		(*ptr) |= (uint32_t)flag->complementary.bool_values.mask;
    }
	else if (flag->complementary.bool_values.ptr_size == sizeof(uint64_t)) {
		uint64_t *ptr = (uint64_t*)flag->data;
		(*ptr) |= (uint64_t)flag->complementary.bool_values.mask;
    }
	else
		return ERROR; // ptr size too big
    return SUCCESS;
}

int dummy_parse(char *val, struct s_flag *flag)
{
    (void)val;
    (void)flag;

    return SUCCESS;
}

int help_parse(char *val, struct s_flag *flag)
{
    (void)val;
    (void)flag;

    return usage(USAGE);
}
int usage_parse(char *val, struct s_flag *flag)
{
    (void)val;
    (void)flag;
    
    print_usage_line();
    return USAGE;
}
