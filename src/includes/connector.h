#pragma once

#include <limits.h>

#include "../hartebeest/src/includes/hartebeest-c.h"

#ifdef __cplusplus
extern "C" {
#endif

// In Bytes
#define RSDCO_BUFSZ         INT_MAX
#define RSCDO_ROOMS_MAX     RSDCO_BUFSZ / 64

#define RSDCO_RESERVED      2
#define RSDCO_LOGIDX_START  (RSDCO_RESERVED - 1)
#define RSDCO_LOGIDX_END    (RSCDO_ROOMS_MAX - 1)
#define RSDCO_AVAIL_ROOMS   (RSCDO_ROOMS_MAX - RSDCO_RESERVED)

struct ConnectionWriteContext {
    struct ibv_qp* local_qp;
    struct ibv_mr* remote_mr;
};

extern struct ibv_mr* replicator_mr;
extern struct ibv_mr* checker_mr;

void rsdco_connector_init();


#ifdef __cplusplus
}
#endif
