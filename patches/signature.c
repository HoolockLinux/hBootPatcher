#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "patches/signature.h"
#include "common.h"
#include "plooshfinder.h"
#include "plooshfinder32.h"

#include "asm/arm64.h"
#include "patches/gadget.h"

uint32_t *sigcheck_ret;

/* OK to match multiple times, as long as it's the same ret */
static bool patch_sigcheck(struct pf_patch_t *patch, uint32_t *stream)
{
    uint32_t *ldp = pf_find_next(stream, 0x190, 0xa9407bfd, 0xffc07fff); // ldp x29, x30, [sp, ...]
    if (!ldp) {
        printf("%s: failed to find image4_validate_property_callback ldp 0x%" PRIx64 "\n", __func__,
               iboot_ptr_to_pa(stream));
        return false;
    }

    uint32_t *retp = pf_find_next(ldp, 10, ret, 0xffffffff);
    if (!retp) {
        uint32_t *shared_b = pf_find_next(ldp, 10, 0x14000000, 0xfc000000);
        if (shared_b) {
            uint32_t *tail = pf_follow_branch(iboot_buf, shared_b);
            retp = pf_find_next(tail, 10, ret, 0xffffffff);
        }
    }

    if (!retp) {
        printf("%s: failed to find image4_validate_property_callback function "
               "return 0x%" PRIx64 "\n",
               __func__, iboot_ptr_to_pa(stream));
        return false;
    }

    if (sigcheck_ret && sigcheck_ret != retp)
        panic("%s: Found twice!\n", __func__);

    if (!sigcheck_ret)
        printf("%s: Found sigcheck\n", __func__);

    sigcheck_ret = retp;

    return true;
}

void signature_patch(void)
{
    /* Find where the EKEY key is matched */
    uint32_t ekey_matches[] = {
        0x5288ab20, // movz w0, #0x4559
        0x72a8a960, // movk w0, #0x454b, lsl 16
    };

    uint32_t ekey_masks[] = {
        0xffffffff,
        0xffffffff,
    };

    struct pf_patch_t sig_check = pf_construct_patch(
        ekey_matches, ekey_masks, sizeof(ekey_matches) / sizeof(uint32_t), (void *)patch_sigcheck);

    /*
     * on older clang, this is reversed
     * Additionally on even older iBoots it may not be w0
     */
    uint32_t ekey_matches_old[] = {
        0x52a8a960, // movz wN, #0x454b, lsl 16
        0x7288ab20, // movk wN, #0x4559
    };

    uint32_t ekey_masks_old[] = {
        0xffffffe0,
        0xffffffe0,
    };

    struct pf_patch_t sig_check_old =
        pf_construct_patch(ekey_matches_old, ekey_masks_old,
                           sizeof(ekey_matches_old) / sizeof(uint32_t), (void *)patch_sigcheck);

    struct pf_patch_t patches[] = {
        sig_check,
        sig_check_old,
    };

    struct pf_patchset_t patchset = pf_construct_patchset(
        patches, sizeof(patches) / sizeof(struct pf_patch_t), (void *)pf_find_maskmatch32);

    pf_patchset_emit(iboot_buf, iboot_len, patchset);
}
