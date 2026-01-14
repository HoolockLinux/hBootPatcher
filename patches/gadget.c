#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "patches/gadget.h"
#include "common.h"
#include "plooshfinder.h"
#include "plooshfinder32.h"

#include "asm/arm64.h"

uint32_t *ret0_gadget;

static bool patch_ret0(struct pf_patch_t *patch, uint32_t *stream)
{
    if (ret0_gadget)
        return false;

    ret0_gadget = stream;
    printf("%s: Found ret0 gadget\n", __func__);
    return true;
}

void gadget_patch(void)
{
    /* Find a ret0 gadget */
    uint32_t ret0_matches[] = {
        0x52800000, // mov w0, #0
        0xd65f03c0, // ret
    };

    uint32_t ret0_masks[] = {
        0xffffffff,
        0xffffffff,
    };

    struct pf_patch_t ret0 = pf_construct_patch(
        ret0_matches, ret0_masks, sizeof(ret0_masks) / sizeof(uint32_t), (void *)patch_ret0);

    struct pf_patch_t patches[] = {
        ret0,
    };

    struct pf_patchset_t patchset = pf_construct_patchset(
        patches, sizeof(patches) / sizeof(struct pf_patch_t), (void *)pf_find_maskmatch32);

    pf_patchset_emit(iboot_buf, iboot_len, patchset);
}
