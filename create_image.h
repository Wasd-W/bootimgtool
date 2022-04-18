#include "bootimg.h"

struct bootimg_params {
    uint32_t kernel_addr;
    uint32_t page_size;
    uint32_t header_version;
    uint32_t tags_addr;
    uint8_t  kernel_filename[50];
    uint8_t  ramdisk_filename[50];
    uint8_t  second_filename[50];
    uint32_t ramdisk_addr;
    uint32_t second_size;
    uint32_t second_addr;
    uint8_t  second_name[50];
    uint32_t os_version;
    uint8_t  cmdline[BOOT_ARGS_SIZE];
    uint8_t  extra_cmdline[BOOT_EXTRA_ARGS_SIZE];
    uint8_t  product_name[BOOT_NAME_SIZE];
    uint32_t id[8];
    uint32_t recovery_dtbo_size;
    uint64_t recovery_dtbo_offset;
    uint32_t header_size;
    uint8_t  dtb_filename[50];
    uint64_t dtb_addr;
};

int create_image(struct bootimg_params *params, const char *filename);
int parse_recipe(int fd, struct bootimg_params *params);
