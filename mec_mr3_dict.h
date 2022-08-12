#pragma once

#include <stdbool.h>
#include <stdint.h>

void check_mec_mr3_dict();
bool check_mec_mr3_info(uint8_t group, uint32_t key, uint32_t type);
const char *get_mec_mr3_info_name(uint8_t group, uint32_t key);
