

#include <vector>
#include <string>

#include "./includes/rsdco_rule.h"

extern int rsdco_sysvar_nid_int;
extern std::vector<std::string> rsdco_sysvar_nids;

int rsdco_rule(uint32_t hashed) {
    return (hashed % rsdco_sysvar_nids.size());
}

int rsdco_rule_always_owner(uint32_t hashed) {
    return rsdco_sysvar_nid_int;
}