#ifndef COMMON_H
#define COMMON_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum image_type {
    IMAGE_TYPE_INVALID = 0,
    IMAGE_TYPE_STAGE1,
    IMAGE_TYPE_STAGE2,
};

#define patch_flag_signature (1 << 0)
#define patch_flag_iorvbar   (1 << 1)
#define patch_flag_recfg     (1 << 2)
#define patch_flag_aes       (1 << 3)

extern uint32_t patch_flags;
extern uint64_t iboot_base;
extern uint8_t *iboot_buf;
extern size_t iboot_len;

extern unsigned char *payload_bin;
enum image_type get_image_type(void);

static inline uint64_t iboot_ptr_to_pa(void *ptr)
{
    return iboot_base + ((uintptr_t)ptr - (uintptr_t)iboot_buf);
}

static inline void *iboot_pa_to_ptr(uint64_t pa)
{
    return (void *)((uintptr_t)iboot_buf + (uintptr_t)(pa - iboot_base));
}

__attribute__((noreturn)) static inline void panic(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    printf("panic: ");
    vprintf(fmt, va);
    va_end(va);
    exit(-1);
}

#endif
