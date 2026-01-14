#ifdef __gnu_linux__
#define _GNU_SOURCE
#endif

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "plooshfinder.h"
#include "plooshfinder32.h"

#include "asm/arm64.h"
#include "patches/aes.h"
#include "patches/gadget.h"
#include "patches/iorvbar.h"
#include "patches/recfg.h"
#include "patches/signature.h"

uint64_t iboot_base;
uint8_t *iboot_buf;
size_t iboot_len;

enum image_type get_image_type(void)
{
    static enum image_type type = IMAGE_TYPE_INVALID;
    if (type)
        return type;

    if (memmem(iboot_buf, iboot_len, "iBoot for", (sizeof("iBoot for") - 1)) ||
        memmem(iboot_buf, iboot_len, "iBootStage2 for", (sizeof("iBootStage2 for") - 1)) ||
        memmem(iboot_buf, iboot_len, "iBEC for", (sizeof("iBEC for") - 1))) {
        type = IMAGE_TYPE_STAGE2;
    } else if (memmem(iboot_buf, iboot_len, "LLB for", (sizeof("LLB for") - 1)) ||
               memmem(iboot_buf, iboot_len, "iBootStage1 for", (sizeof("iBootStage1 for") - 1)) ||
               memmem(iboot_buf, iboot_len, "iBSS for", (sizeof("iBSS for") - 1))) {
        type = IMAGE_TYPE_STAGE1;
    }
    return type;
}

/*
 * perform some checkra1n patches to bring environment closer
 * to booting from pongoOS (optional)
 */
int patch_ibootstage2(void)
{
    iorvbar_patch();
    aes_patch();

    if (memmem(iboot_buf, iboot_len, "arm-io/aop", sizeof("arm-io/aop"))) {
        recfg_patch();

        if (!did_patch_recfg) {
            printf("recfg patch not found!\n");
            return -1;
        }
    } else {
        printf("%s: Target does not have recfg, skipping...\n", __func__);
    }

    if (!did_patch_aes) {
        printf("aes patch not found!\n");
        return -1;
    }

    if (!did_patch_iorvbar) {
        printf("iorvbar patch not found!\n");
        return -1;
    }

    return 0;
}

int patch_iboot(void)
{
    printf("starting hBootPatcher\n");

    uint32_t *ldr = pf_find_next((uint32_t *)iboot_buf, 10, 0x58000001, 0xff00001f);
    if (!ldr) {
        printf("%s: failed to find initial ldr!\n", __func__);
        return -1;
    }

    uint32_t ldr_off = (ldr[0] >> 5) & 0x7ffff; // uint32_t takes care of << 2
    uint64_t *iboot_base_p = (uint64_t *)(ldr + ldr_off);

    iboot_base = iboot_base_p[0];
    uint64_t iboot_end = iboot_base_p[1];

    printf("%s: iboot_base = 0x%" PRIx64 "\n", __func__, iboot_base);
    printf("%s: iboot_end = 0x%" PRIx64 "\n", __func__, iboot_end);

    if (iboot_base >= iboot_end) {
        printf("iboot_end is not bigger than iboot_base!\n");
        return -1;
    }

    switch (get_image_type()) {
        case IMAGE_TYPE_STAGE1:
            printf("%s: Image is stage1\n", __func__);
            break;
        case IMAGE_TYPE_STAGE2:
            printf("%s: Image is stage2\n", __func__);
            break;
        case IMAGE_TYPE_INVALID:
            printf("%s: failed to determine image type\n", __func__);
            return -1;
    }

    gadget_patch();

    if (!ret0_gadget) {
        printf("%s: ret0 gadget not found!\n", __func__);
        return -1;
    }

    signature_patch();

    if (!sigcheck_ret) {
        printf("could not patch signature check\n");
        return -1;
    } else {
        sigcheck_ret[0] = arm64_branch(sigcheck_ret, ret0_gadget, false);
    }

    if (get_image_type() == IMAGE_TYPE_STAGE2 && patch_ibootstage2())
        return -1;

    return 0;
}

int main(int argc, char **argv)
{
    FILE *fp = NULL;

    if (argc < 3) {
        printf("Usage: %s <input iboot> <patched iboot>\n", argv[0]);
        return 0;
    }

    fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("Failed to open iboot!\n");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    iboot_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (iboot_len < 0x4000) {
        printf("iBoot too small\n");
        fclose(fp);
        return -1;
    }

    iboot_buf = (void *)malloc(iboot_len);
    if (!iboot_buf) {
        printf("Out of memory while allocating region for iboot!\n");
        fclose(fp);
        return -1;
    }

    fread(iboot_buf, 1, iboot_len, fp);
    fclose(fp);

    int retval = patch_iboot();

    if (retval) {
        printf("patchfinding failed!\n");
        return retval;
    }

    fp = fopen(argv[2], "wb");
    if (!fp) {
        printf("Failed to open output file!\n");
        free(iboot_buf);
        return -1;
    }

    fwrite(iboot_buf, 1, iboot_len, fp);
    fflush(fp);
    fclose(fp);

    free(iboot_buf);

    return retval;
}
