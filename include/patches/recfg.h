#ifndef RECFG_H
#define RECFG_H

#include <stdint.h>
#include <stdbool.h>

void recfg_patch(void);

extern bool did_patch_recfg, has_recfg;

#endif
