#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "patches/recfg.h"
#include "common.h"
#include "plooshfinder.h"
#include "plooshfinder32.h"

#include "asm/arm64.h"
#include "patches/gadget.h"

bool did_patch_recfg;
bool has_recfg = true;

static bool patch_recfg(struct pf_patch_t *patch, uint32_t *stream)
{
    uint32_t *lock = pf_follow_branch(iboot_buf, &stream[5]);
    if (!lock)
        panic("%s: failed to follow branch!\n", __func__);

    /* patch lock to ret */
    lock[0] = arm64_branch(lock, ret0_gadget, false);

    printf("%s: Found recfg\n", __func__);
    did_patch_recfg = true;
    return true;
}

void recfg_patch(void)
{
    /*
     * https://github.com/palera1n/PongoOS/blob/b1038fd75a2608aa8dbc0dd3c94a0f1f97118c28/src/boot/patches.S#L299
     */

    uint32_t recfg_old_matches[] = {
        0x320007e0, // orr w0, wzr, 3
        0x94000000, // bl 0x...
        0x320007e0, // orr w0, wzr, 3
        0x94000000, // bl 0x...
        0x320007e0, // orr w0, wzr, 3
        0x94000000, // bl 0x...
    };

    uint32_t recfg_old_masks[] = {
        0xffffffff, //
        0xfc000000, //
        0xffffffff, //
        0xfc000000, //
        0xffffffff, //
        0xfc000000, //
    };

    struct pf_patch_t recfg_old =
        pf_construct_patch(recfg_old_matches, recfg_old_masks,
                           sizeof(recfg_old_masks) / sizeof(uint32_t), (void *)patch_recfg);

    uint32_t recfg_new_matches[] = {
        0x52800060, // mov w0, #3
        0x94000000, // bl
        0x52800060, // mov w0, #3
        0x94000000, // bl
        0x52800060, // mov w0, #3
        0x94000000, // bl
    };

    uint32_t recfg_new_masks[] = {
        0xffffffff, //
        0xfc000000, //
        0xffffffff, //
        0xfc000000, //
        0xffffffff, //
        0xfc000000, //
    };

    struct pf_patch_t recfg_new =
        pf_construct_patch(recfg_new_matches, recfg_new_masks,
                           sizeof(recfg_new_masks) / sizeof(uint32_t), (void *)patch_recfg);

    struct pf_patch_t patches[] = {
        recfg_old,
        recfg_new,
    };

    struct pf_patchset_t patchset = pf_construct_patchset(
        patches, sizeof(patches) / sizeof(struct pf_patch_t), (void *)pf_find_maskmatch32);

    pf_patchset_emit(iboot_buf, iboot_len, patchset);
}
