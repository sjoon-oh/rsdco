#pragma once

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

void rsdco_spawn_receiver(void (*)(void*, uint16_t));

#ifdef __cplusplus
}
#endif
