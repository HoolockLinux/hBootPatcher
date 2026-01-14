#ifndef AES_H
#define AES_H

#include <stdint.h>
#include <stdbool.h>

void aes_patch(void);
extern bool did_patch_aes;

#endif
