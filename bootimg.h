#ifndef BOOTIMG_H
#define BOOTIMG_H

#include "types.h"

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE      8
#define BOOT_NAME_SIZE       16
#define BOOT_ARGS_SIZE       512
#define BOOT_EXTRA_ARGS_SIZE 1024

struct bootimg_hdr_0_2 {
    /* v0 */
    uint8_t  magic[BOOT_MAGIC_SIZE];
    uint32_t kernel_size;
    uint32_t kernel_addr;

    uint32_t ramdisk_size;
    uint32_t ramdisk_addr;

    uint32_t second_size;
    uint32_t second_addr;

    uint32_t tags_addr;
    uint32_t page_size;

    uint32_t header_version;
    uint32_t os_version;
    uint8_t  name[BOOT_NAME_SIZE];
    uint8_t  cmdline[BOOT_ARGS_SIZE];
    uint32_t id[8];
    uint8_t  extra_cmdline[BOOT_EXTRA_ARGS_SIZE];

    /* v1 */
    uint32_t recovery_dtbo_size;
    uint64_t recovery_dtbo_offset;
    uint32_t header_size;

    /* v2 */
    uint32_t dtb_size;
    uint64_t dtb_addr;
} __attribute__((packed));

#define EXTRA_BYTES_v1 12
#define EXTRA_BYTES_v2 (EXTRA_BYTES_v1 + 16)

#endif
