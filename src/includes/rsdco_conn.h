#pragma once

#include <limits.h>

#include <infiniband/verbs.h>
#ifdef __cplusplus
extern "C" {
#endif

// In Bytes
// const int rsdco_buf_sz = INT_MAX

#define RSDCO_BUFSZ         2147483647
#define RSCDO_ROOMS_MAX     (RSDCO_BUFSZ >> 5)

#define RSDCO_RESERVED      2
#define RSDCO_LOGIDX_START  (RSDCO_RESERVED)
#define RSDCO_LOGIDX_END    (RSCDO_ROOMS_MAX - 1)
#define RSDCO_AVAIL_ROOMS   (RSCDO_ROOMS_MAX - RSDCO_RESERVED)

struct RsdcoConnectPair {
    struct ibv_qp* local_qp;
    struct ibv_mr* remote_mr;
    int remote_node_id;
};

void rsdco_connector_init();
const int rsdco_get_node_id();

#ifdef __cplusplus
}
#endif
