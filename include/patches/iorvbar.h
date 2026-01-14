#ifndef IORVBAR_H
#define IORVBAR_H

#include <stdint.h>
#include <stdbool.h>

void iorvbar_patch(void);
extern bool did_patch_iorvbar;

#endif
