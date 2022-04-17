#include <fcntl.h>
#include <openssl/sha.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "create_image.h"

static uint32_t align(uint32_t value)
{
    unsigned int alignment_mask = 4 - 1;
    return (value + alignment_mask) & ~alignment_mask;
}

static uint8_t *load_file(const char *filename, uint32_t *file_size)
{
    int fd = 0;
    uint8_t *data = NULL;

    fd = open(filename, O_RDONLY);

    if(fd > 0)
    {
        *file_size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);
        data = malloc(*file_size);
        if(read(fd, data, *file_size) < 0) {
            free(data);
            data = NULL;
        }
        close(fd);
    }
    return data;
}

static void write_padding(int fd, uint32_t count)
{
    uint8_t *padding = malloc(count);
    memset(padding, 0, count);
    write(fd, padding, count);
    free(padding);
}

int create_image(struct bootimg_params *params, const char *filename)
{
    int fd = 0;
    struct bootimg_hdr_0_2 hdr;
    uint32_t header_size = 0;
    uint32_t kernel_size = 0;
    uint32_t ramdisk_size = 0;
    uint32_t second_size = 0;
    uint32_t dtb_size = 0;
    uint8_t *kernel_data = NULL;
    uint8_t *ramdisk_data = NULL;
    uint8_t *second_data = NULL;
    uint8_t *dtb_data = NULL;

    if(!strcmp((filename + (strlen(filename) - 4)), ".img")) {
        fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    } else {
        char *new_filename = malloc(strlen(filename) + 5);

        memset(new_filename, 0, strlen(filename) + 5);
        memcpy(new_filename, filename, strlen(filename));
        strcpy(new_filename + strlen(filename), ".img");
        fd = open(new_filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
        free(new_filename);
    }

    memset(&hdr, 0, sizeof(struct bootimg_hdr_0_2));

    memcpy(hdr.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);
    
    hdr.kernel_addr = params->kernel_addr;
    hdr.ramdisk_addr = params->ramdisk_addr;
    hdr.page_size = params->page_size;
    hdr.header_version = params->header_version;
    hdr.os_version = params->os_version;
    hdr.tags_addr = params->tags_addr;
    memcpy(hdr.cmdline, params->cmdline, BOOT_ARGS_SIZE);
    memcpy(hdr.extra_cmdline, params->extra_cmdline, BOOT_EXTRA_ARGS_SIZE);
    memcpy(hdr.name, params->product_name, BOOT_NAME_SIZE);

    kernel_data = load_file(params->kernel_filename, &kernel_size);

    if(kernel_data == NULL)
    {
        fprintf(stderr, "FATAL: could not find kernel file\n");
        close(fd);
        return 1;
    }

    hdr.kernel_size = kernel_size;

    ramdisk_data = load_file(params->ramdisk_filename, &ramdisk_size);

    if(ramdisk_data == NULL)
    {
        fprintf(stderr, "FATAL: could not find ramdisk file\n");
        free(kernel_data);
        close(fd);
        return 1;
    }

    hdr.ramdisk_size = ramdisk_size;

    uint32_t aligned_ramdisk_size = align(hdr.ramdisk_size);

    if(aligned_ramdisk_size != hdr.ramdisk_size)
    {
        fprintf(stdout, "current ramdisk not aligned: %lu\n", hdr.ramdisk_size);
        uint32_t remainder = aligned_ramdisk_size - hdr.ramdisk_size;
        uint8_t *temp_ramdisk_data = realloc(ramdisk_data, hdr.ramdisk_size + remainder);

        if(temp_ramdisk_data != NULL)
        {
            ramdisk_data = temp_ramdisk_data;
            memset(ramdisk_data + hdr.ramdisk_size, 0, remainder);
            hdr.ramdisk_size += remainder;
            fprintf(stdout, "new aligned size: %lu\n", hdr.ramdisk_size);
        }
    }

    if(params->header_version > 1) {
        header_size = sizeof(struct bootimg_hdr_0_2);
    } else if(params->header_version > 0) {
        header_size = sizeof(struct bootimg_hdr_0_2) - EXTRA_BYTES_v1;
    } else if(params->header_version == 0) {
        header_size = sizeof(struct bootimg_hdr_0_2) - EXTRA_BYTES_v2;
    }

    if(params->second_addr != 0)
    {
        second_data = load_file("second", &second_size);

        if(second_data != NULL)
        {
            hdr.second_size = second_size;
            hdr.second_addr = params->second_addr;
        }
    }

    if(params->header_version > 1)
    {
        dtb_data = load_file(params->dtb_filename, &dtb_size);
        hdr.dtb_size = dtb_size;
        hdr.dtb_addr = params->dtb_addr;
    }

    if(params->header_version > 0)
    {
        hdr.header_size = header_size;

        if(params->recovery_dtbo_size != 0)
        {
            /* TODO: Load recovery data */
        }
    }

    if(params->keep_id == 1)
    {
        memcpy(hdr.id, params->id, sizeof(uint32_t) * 8);
    }
    else
    {
        SHA_CTX c;
        uint8_t sha[SHA_DIGEST_LENGTH];

        SHA1_Init(&c);

        SHA1_Update(&c, kernel_data, hdr.kernel_size);
        SHA1_Update(&c, &hdr.kernel_size, sizeof(hdr.kernel_size));
        SHA1_Update(&c, ramdisk_data, hdr.ramdisk_size);
        SHA1_Update(&c, &hdr.ramdisk_size, sizeof(hdr.ramdisk_size));

        if(second_size != 0)
        {
            SHA1_Update(&c, second_data, hdr.second_size);
            SHA1_Update(&c, &hdr.second_size, sizeof(hdr.second_size));
        }

        SHA1_Update(&c, &hdr.tags_addr, sizeof(hdr.tags_addr));
        SHA1_Update(&c, &hdr.page_size, sizeof(hdr.page_size));
        SHA1_Update(&c, &hdr.header_version, sizeof(hdr.header_version));
        SHA1_Update(&c, &hdr.os_version, sizeof(hdr.os_version));
        SHA1_Update(&c, hdr.name, sizeof(hdr.name));
        SHA1_Update(&c, hdr.cmdline, sizeof(hdr.cmdline));
        
        SHA1_Final(sha, &c);
        memcpy(hdr.id, sha, SHA_DIGEST_LENGTH > sizeof(hdr.id) ? sizeof(hdr.id) : SHA_DIGEST_LENGTH);
    }

    write(fd, &hdr, header_size);
    write_padding(fd, params->page_size - header_size);
    write(fd, kernel_data, hdr.kernel_size);
    free(kernel_data);
    write_padding(fd, (params->page_size - (hdr.kernel_size % params->page_size)));
    write(fd, ramdisk_data, hdr.ramdisk_size);
    write_padding(fd, params->page_size - (hdr.ramdisk_size % params->page_size));
    free(ramdisk_data);

    if(second_size > 0)
    {
        write(fd, second_data, second_size);
        write_padding(fd, params->page_size - (second_size % params->page_size));
        free(second_data);
    }

    /* TODO: load recovery */

    if(dtb_data != NULL)
    {
        write(fd, dtb_data, dtb_size);
        write_padding(fd, params->page_size - (dtb_size % params->page_size));
        free(dtb_data);
    }

    close(fd);
    return 0;
}

int parse_recipe(int fd, struct bootimg_params *params)
{
    int     bytes_read = 0;
    uint8_t *key = malloc(4);

    memset(key, 0, 4);

    while((bytes_read = read(fd, key, 3)) > 0) {
        if(!strcmp(key, "kna")) {
            uint32_t kernel_addr = 0;
            read(fd, &kernel_addr, sizeof(uint32_t));
            params->kernel_addr = kernel_addr;
        } else if(!strcmp(key, "knn")) {
            uint32_t len = 0;
            read(fd, &len, sizeof(uint32_t));
            read(fd, params->kernel_filename, len);
        } else if(!strcmp(key, "pas")) {
            uint32_t page_size = 0;
            read(fd, &page_size, sizeof(uint32_t));
            params->page_size = page_size;
        } else if(!strcmp(key, "hev")) {
            uint32_t header_version = 0;
            read(fd, &header_version, sizeof(uint32_t));
            params->header_version = header_version;
        } else if(!strcmp(key, "rda")) {
            uint32_t ramdisk_addr = 0;
            read(fd, &ramdisk_addr, sizeof(uint32_t));
            params->ramdisk_addr = ramdisk_addr;
        } else if(!strcmp(key, "osv")) {
            uint32_t os_version = 0;
            read(fd, &os_version, sizeof(uint32_t));
            params->os_version = os_version;
        } else if(!strcmp(key, "taa")) {
            uint32_t tags_addr = 0;
            read(fd, &tags_addr, sizeof(uint32_t));
            params->tags_addr = tags_addr;
        } else if(!strcmp(key, "rdn")) {
            uint32_t len = 0;
            read(fd, &len, sizeof(uint32_t));
            read(fd, params->ramdisk_filename, len);
        } else if(!strcmp(key, "sea")) {
            uint32_t second_addr = 0;
            read(fd, &second_addr, sizeof(uint32_t));
            params->second_addr = second_addr;
        } else if(!strcmp(key, "cmd")) {
            uint32_t len = 0;
            read(fd, &len, sizeof(uint32_t));
            read(fd, params->cmdline, len);
        } else if(!strcmp(key, "ecm")) {
            uint32_t len = 0;
            read(fd, &len, sizeof(uint32_t));
            read(fd, params->extra_cmdline, len);
        } else if(!strcmp(key, "pna")) {
            uint32_t len = 0;
            read(fd, &len, sizeof(uint32_t));
            read(fd, params->product_name, len);
        } else if(!strcmp(key, "sen")) {
            uint32_t len = 0;
            read(fd, &len, sizeof(uint32_t));
            read(fd, params->second_filename, len);
        } else if(!strcmp(key, "idv")) {
            read(fd, params->id, sizeof(uint32_t) * 8);
        } else if(!strcmp(key, "dtn")) {
            uint32_t len = 0;
            read(fd, &len, sizeof(uint32_t));
            read(fd, params->dtb_filename, len);
        } else if(!strcmp(key, "reo")) {
            uint64_t offset = 0;
            read(fd, &offset, sizeof(uint64_t));
            params->recovery_dtbo_offset = offset;
        } else if(!strcmp(key, "dta")) {
            uint64_t addr = 0;
            read(fd, &addr, sizeof(uint64_t));
            params->dtb_addr = addr;
        }
    }

    free(key);
    close(fd);
    return 0;
}
