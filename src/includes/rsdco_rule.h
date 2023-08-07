#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int rsdco_rule_balanced(uint32_t);
int rsdco_rule_always_owner(uint32_t);
int rsdco_rule_skew_single(uint32_t);

#ifdef __cplusplus
}
#endif