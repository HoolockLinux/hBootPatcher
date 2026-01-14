#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <stdint.h>

void signature_patch(void);
extern uint32_t *sigcheck_ret;

#endif
