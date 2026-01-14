#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "patches/iorvbar.h"
#include "common.h"
#include "plooshfinder.h"
#include "plooshfinder32.h"

#include "asm/arm64.h"

bool did_patch_iorvbar;

bool patch_iorvbar(struct pf_patch_t *patch, uint32_t *stream)
{
    if (did_patch_iorvbar)
        panic("%s: Found twice!\n", __func__);

    // orr xM, xN, ... -> orr xM, xN, xzr
    stream[0] = 0xaa1f0000 | (stream[0] & 0x3ff);

    did_patch_iorvbar = true;
    printf("%s: Found IORVBAR\n", __func__);
    return true;
}

bool patch_iorvbar_old(struct pf_patch_t *patch, uint32_t *stream)
{
    uint32_t and_reg = stream[0] & 0x1f;
    uint32_t orr_reg = (stream[1] >> 5) & 0x1f;

    if (and_reg != orr_reg)
        return false;

    return patch_iorvbar(patch, &stream[1]);
}

bool patch_iorvbar_new(struct pf_patch_t *patch, uint32_t *stream)
{
    uint32_t and_xN = stream[0] & 0x1f;
    uint32_t orr_xN = (stream[2] >> 5) & 0x1f;

    if (and_xN != orr_xN)
        return false;

    uint32_t orr_xK = (stream[2] >> 16) & 0x1f;
    uint32_t mov_wK = (stream[1] & 0x1f);

    if (orr_xK != mov_wK)
        return false;

    return patch_iorvbar(patch, &stream[2]);
}

void iorvbar_patch(void)
{
    // https://github.com/palera1n/PongoOS/blob/b1038fd75a2608aa8dbc0dd3c94a0f1f97118c28/src/boot/patches.S#L51

    uint32_t iorvbar_old_match[] = {
        0x9275d000, // and xN, x0, 0xfffffffffffff800
        0xb2400000, // orr xM, xN, 1
    };

    uint32_t iorvbar_old_masks[] = {
        0xffffffe0,
        0xfffffc00,
    };

    struct pf_patch_t iorvbar_old =
        pf_construct_patch(iorvbar_old_match, iorvbar_old_masks,
                           sizeof(iorvbar_old_masks) / sizeof(uint32_t), (void *)patch_iorvbar_old);

    uint32_t iorvbar_new_match[] = {
        0x9275d000, // and xN, x0, 0xfffffffffffff800
        0x2a0103e0, // mov wK, w1
        0xaa000000, // orr xM, xN, xK
    };

    uint32_t iorvbar_new_masks[] = {
        0xffffffe0,
        0xffffffe0,
        0xffe0fc00,
    };

    struct pf_patch_t iorvbar_new =
        pf_construct_patch(iorvbar_new_match, iorvbar_new_masks,
                           sizeof(iorvbar_new_masks) / sizeof(uint32_t), (void *)patch_iorvbar_new);

    struct pf_patch_t patches[] = {
        iorvbar_old,
        iorvbar_new,
    };

    struct pf_patchset_t patchset = pf_construct_patchset(
        patches, sizeof(patches) / sizeof(struct pf_patch_t), (void *)pf_find_maskmatch32);

    pf_patchset_emit(iboot_buf, iboot_len, patchset);
}
