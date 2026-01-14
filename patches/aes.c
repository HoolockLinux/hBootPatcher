#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "patches/aes.h"
#include "common.h"
#include "plooshfinder.h"
#include "plooshfinder32.h"

#include "asm/arm64.h"
#include "patches/gadget.h"

bool did_patch_aes;

bool patch_aes(struct pf_patch_t *patch, uint32_t *stream)
{
    uint32_t *bl1 = pf_follow_branch(iboot_buf, &stream[1]);
    uint32_t *bl4 = pf_follow_branch(iboot_buf, &stream[4]);

    if (bl1 != bl4)
        return false;

    uint32_t *bl_disable = pf_find_next(stream, 0x10, 0x94000000, 0xfc000000);
    if (!bl_disable) {
        printf("%s: Failed to find bl platform_disable_keys\n", __func__);
        return false;
    }

    if (did_patch_aes)
        panic("%s: Found twice!\n");

    uint32_t *disable = pf_follow_branch(iboot_buf, bl_disable);
    disable[0] = arm64_branch(disable, ret0_gadget, false);

    printf("%s: Found AES\n", __func__);
    did_patch_aes = true;
    return true;
}

void aes_patch(void)
{
    // https://github.com/palera1n/PongoOS/blob/b1038fd75a2608aa8dbc0dd3c94a0f1f97118c28/src/boot/patches.S#L162
    uint32_t aes_matches_old[] = {
        0x320e03e0, // orr w0, wzr, 0x40000
        0x94000000, // bl security_allow_modes
        0xaa0003f0, // mov x{16-31}, x0
        0x320d03e0, // orr w0, wzr, 0x80000
        0x94000000, // bl security_allow_modes
    };

    uint32_t aes_masks_old[] = {
        0xffffffff, 0xfc000000, 0xfffffff0, 0xffffffff, 0xfc000000,
    };

    struct pf_patch_t aes_old =
        pf_construct_patch(aes_matches_old, aes_masks_old, sizeof(aes_masks_old) / sizeof(uint32_t),
                           (void *)patch_aes);

    uint32_t aes_matches_new[] = {
        0x52a00080, // mov w0, 0x40000
        0x94000000, // bl security_allow_modes
        0xaa0003f0, // mov x{16-31}, x0
        0x52a00100, // mov w0, 0x80000
        0x94000000, // bl security_allow_modes
    };

    uint32_t aes_masks_new[] = {
        0xffffffff, 0xfc000000, 0xfffffff0, 0xffffffff, 0xfc000000,
    };

    struct pf_patch_t aes_new =
        pf_construct_patch(aes_matches_new, aes_masks_new, sizeof(aes_masks_new) / sizeof(uint32_t),
                           (void *)patch_aes);

    struct pf_patch_t patches[] = {
        aes_old,
        aes_new,
    };

    struct pf_patchset_t patchset = pf_construct_patchset(
        patches, sizeof(patches) / sizeof(struct pf_patch_t), (void *)pf_find_maskmatch32);

    pf_patchset_emit(iboot_buf, iboot_len, patchset);
}
