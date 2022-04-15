/*
 * bootimgtool
 * ===========
 *
 * The tool will have three modes of operation: info, create, disassemble.
 *
 * info: will show information from the header.
 * create: will create a new boot.img according to the given parameters, or to a
 * recipe.cfg file if it exists.
 * disassemble: will give you kernel, ramdisk, second, etc. and will create a
 * recipe.cfg file, to be able to recreate the image.
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bootimgtool.h"
#include "create_image.h"

int  is_valid_image(int fd)
{
    int file_size = 0;
    uint8_t magic[BOOT_MAGIC_SIZE];

    file_size = lseek(fd, 0, SEEK_END);
    if(file_size < sizeof(struct bootimg_hdr_0_2)) {
        return -1;
    }
    lseek(fd, 0, SEEK_SET);

    if(pread(fd, magic, BOOT_MAGIC_SIZE, 0) < 0) {
        return -1;
    }

    if(strncmp(magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
        return  -1;
    }
    return 1;
}

char* get_os_patch_level(uint32_t os_patch_level)
{
    char *patch_level = malloc(6);
    memset(patch_level, 0, 6);

    uint32_t y = ((os_patch_level >> 4) & 0x7f) + 2000;
    uint32_t m = (os_patch_level & 0xf);
    sprintf(patch_level, "%d-%.2d", y, m);
    return patch_level;
}

char* get_os_version(uint32_t os_version)
{
    char *version = malloc(9);
    memset(version, 0, 9);

    uint32_t release = (os_version >> 25) & 0x7f;
    uint32_t major   = (os_version >> 18) & 0x7f;
    uint32_t minor   = (os_version >> 11) & 0x7f;

    sprintf(version, "%d.%d.%d", release, major, minor);
    return version;
}

int  read_header(int fd, struct bootimg_hdr_0_2 *header)
{
    if(pread(fd, header, sizeof(struct bootimg_hdr_0_2), 0) < 0)  {
        return -1;
    }
    return 1;
}

void show_info(struct bootimg_hdr_0_2 *header)
{
    char *version  = get_os_version(header->os_version);
    char *patch_level = get_os_patch_level(header->os_version);

    fprintf(stdout, "header version: %d\n", header->header_version);
    fprintf(stdout, "kernel size = %d\n", header->kernel_size);
    fprintf(stdout, "kernel address = 0x%x\n", header->kernel_addr);
    fprintf(stdout, "ramdisk size = %d\n", header->ramdisk_size);
    fprintf(stdout, "ramdisk address = 0x%x\n", header->ramdisk_addr);
    
    if(header->second_size > 0) {
        fprintf(stdout, "second size = %d\n", header->second_size);
        fprintf(stdout, "second address = 0x%x\n", header->second_addr);
    }

    fprintf(stdout, "tags address = 0x%x\n", header->tags_addr);
    fprintf(stdout, "os version = %s\n", version);
    fprintf(stdout, "os patch level = %s\n", patch_level);
    free(version);
    free(patch_level);
    fprintf(stdout, "name = %s\n", header->name);
    fprintf(stdout, "cmdline = %s\n", header->cmdline);
    fprintf(stdout, "pagesize = %d\n", header->page_size);

    if(header->header_version > 0) {
        fprintf(stdout, "header size = %u\n", header->header_size);

        if(header->recovery_dtbo_size > 0) {
            fprintf(stdout, "recovery dtbo size = %u\n", header->recovery_dtbo_size);
            fprintf(stdout, "recovery dtbo offset = %lu\n", header->recovery_dtbo_offset);
        }
    }

    if(header->header_version > 1) {
        fprintf(stdout, "dtb size = %u\n", header->dtb_size);
        fprintf(stdout, "dtb addr = 0x%x\n", header->dtb_addr);
    }
}

int usage()
{
    fprintf(stdout, "Usage: bootimgtool [info | create | disassemble]\n");
    return 1;
}

void write_to_recipe(enum rtypes type, void *value, int fd)
{
    char  *key  = NULL;
    uint32_t size = 0;

    switch(type) {
        case RTYPE_KNN:
        case RTYPE_RDN:
        case RTYPE_SEN:
        case RTYPE_CMD:
        case RTYPE_PNA:
        case RTYPE_ECM:
        case RTYPE_DTN:
            size = strlen((char*) value) + 1;
            key = malloc(3 + sizeof(uint32_t));

            if(type == RTYPE_KNN)
                key[0] = 'k', key[1] = 'n', key[2] = 'n';
            if(type == RTYPE_RDN)
                key[0] = 'r', key[1] = 'd', key[2] = 'n';
            if(type == RTYPE_SEN)
                key[0] = 's', key[1] = 'e', key[2] = 'n';
            if(type == RTYPE_CMD)
                key[0] = 'c', key[1] = 'm', key[2] = 'd';
            if(type == RTYPE_PNA)
                key[0] = 'p', key[1] = 'n', key[2] = 'a';
            if(type == RTYPE_ECM)
                key[0] = 'e', key[1] = 'c', key[2] = 'm';
            if(type == RTYPE_DTN)
                key[0] = 'd', key[1] = 't', key[2] = 'n';
            
            memcpy(key + 3, &size, sizeof(uint32_t));
            write(fd, key, 3 + sizeof(uint32_t));
            free(key);
            break;
        case RTYPE_KNA:
            key = "kna";
            size = sizeof(uint32_t);
            write(fd, key, 3);
            break;
        case RTYPE_PAS:
            key = "pas";
            size = sizeof(uint32_t);
            write(fd, key, 3);
            break;
        case RTYPE_RDA:
            key = "rda";
            size = sizeof(uint32_t);
            write(fd, key, 3);
            break;
        case RTYPE_SEA:
            key = "sea";
            size = sizeof(uint32_t);
            write(fd, key, 3);
            break;
        case RTYPE_HEV:
            key = "hev";
            size = sizeof(uint32_t);
            write(fd, key, 3);
            break;
        case RTYPE_OSV:
            key = "osv";
            size = sizeof(uint32_t);
            write(fd, key, 3);
            break;
        case RTYPE_IDV:
            key = "idv";
            size = sizeof(uint32_t) * 8;
            write(fd, key, 3);
            break;
        case RTYPE_REO:
            key = "reo";
            size = sizeof(uint64_t);
            write(fd, key, 3);
            break;
        case RTYPE_DTA:
            key = "dta";
            size = sizeof(uint64_t);
            write(fd, key, 3);
            break;
        case RTYPE_TAA:
            key = "taa";
            size = sizeof(uint32_t);
            write(fd, key, 3);
            break;
        default:
            return;
    }
    write(fd, value, size);
}

int main(int argc, char *argv[])
{
    if(argc >= 2) 
    {
        if(!strcmp(argv[1], "info")) 
        {
            if(argc >= 3) 
            {
                const char*            filename = argv[2];
                int                    fd = 0;
                struct bootimg_hdr_0_2 hdr;

                if((fd = open(filename, O_RDONLY)) != -1) 
                {
                    if(is_valid_image(fd) > 0) 
                    {
                        memset(&hdr, 0, sizeof(struct bootimg_hdr_0_2));

                        if(read_header(fd, &hdr) > 0) 
                        {
                            if(hdr.header_version > 2) 
                            {
                                fprintf(stderr, "info: Unsupported header version: %u\n", hdr.header_version);
                                close(fd);
                                return 1;
                            }

                            show_info(&hdr);
                            close(fd);
                        } 
                        else 
                        {
                            fprintf(stderr, "info: could not read header from %s\n", filename);
                            close(fd);
                        }
                    } 
                    else 
                    {
                        fprintf(stderr, "%s is not a valid image\n", filename);
                        return 1;
                    }
                } 
                else 
                {
                    fprintf(stderr, "info: could not open file %s\n", filename);
                    return 1;
                }
            } 
            else 
            {
                fprintf(stderr, "info: need an image file\n");
                return 1;
            }
        } 
        else if(!strcmp(argv[1], "create")) 
        {
            struct bootimg_params params;
            int                   fd = 0;
            char                  *filename = NULL;

            memset(&params, 0, sizeof(struct bootimg_params));

            fd = open("recipe.cfg", O_RDONLY);

            if(fd != -1) 
            {
                char **ars = argv + 2;
                int  arc = argc - 2;
                char *filename = NULL;

               if(arc > 1) 
               {
                   while(arc > 0)
                   {
                       if(!strcmp(*ars, "-o") || !strcmp(*ars, "--output"))
                        {
                            filename = *(ars + 1);
                            ars += 2;
                            arc -= 2;
                        }
                        else if(!strcmp(*ars, "-k") || !strcmp(*ars, "--keep-id"))
                        {
                            params.keep_id = 1;
                            arc -= 1;
                            ars += 1;
                        }
                        else
                        {
                            fprintf(stderr, "create: unknown flag %s\n", *ars);
                            return 1;
                        }

                   }
                   if(filename == NULL)
                   {
                       fprintf(stderr, "create: need to specify an  output filename\n");
                       return 1;
                   }
                   parse_recipe(fd, &params);
                   create_image(&params, filename);
                   /* if(!strcmp(*ars, "-o") || !strcmp(*ars, "--output")) 
                   {
                       filename = *(ars + 1);
                       parse_recipe(fd, &params);
                       create_image(&params, filename);
                   } 
                   else 
                   {
                       fprintf(stderr, "create: unknown option %s\n", *ars);
                       close(fd);
                       return 1;
                   } */
               } 
               else 
               {
                   fprintf(stderr, "create: insufficient parameters\n");
                   close(fd);
                   return 1;
               }
            } 
            else 
            {
                /* TODO: create the image manually */
            }
        } 
        else if(!strcmp(argv[1], "disassemble")) 
        {
            if(argc >= 3) 
            {
                int                    fd = open(argv[2], O_RDONLY);
                struct bootimg_hdr_0_2 hdr;

                memset(&hdr, 0, sizeof(struct bootimg_hdr_0_2));
                
                if(fd != -1) 
                {
                    if(read_header(fd, &hdr) > 0) 
                    {
                        if(hdr.header_version > 2) 
                        {
                            fprintf(stderr, "disassemble: unsupported header version %u\n", hdr.header_version);
                            close(fd);
                            return 1;
                        }

                        int recipe_fd = open("recipe.cfg", O_RDWR | O_CREAT | O_TRUNC, 0644);

                        if(recipe_fd != -1) 
                        {
                            uint8_t *kernel_data = NULL;
                            uint8_t *kernel_filename = NULL;
                            uint8_t *ramdisk_data = NULL;
                            uint8_t *ramdisk_filename = NULL;
                            uint8_t *second_data = NULL;
                            uint32_t kernel_pages = (hdr.kernel_size + hdr.page_size - 1) / hdr.page_size;
                            uint32_t ramdisk_pages = (hdr.ramdisk_size + hdr.page_size - 1) / hdr.page_size;
                            uint32_t second_pages = 0;
                            uint32_t recovery_pages = 0;
                            int kernel_fd = 0;
                            int ramdisk_fd = 0;
                            int second_fd = 0;
                            int recovery_fd = 0;
                            int dtb_fd = 0;

                            write_to_recipe(RTYPE_KNA, &hdr.kernel_addr, recipe_fd);
                            write_to_recipe(RTYPE_PAS, &hdr.page_size, recipe_fd);
                            write_to_recipe(RTYPE_HEV, &hdr.header_version, recipe_fd);
                            write_to_recipe(RTYPE_TAA, &hdr.tags_addr, recipe_fd);

                            kernel_data = malloc(hdr.kernel_size);
                            pread(fd, kernel_data, hdr.kernel_size, hdr.page_size);

                            if(kernel_data[0] == 0x1f && kernel_data[1] == 0x8b) 
                            {
                                kernel_filename = "kernel.gz";
                            } 
                            else 
                            {
                                kernel_filename = "kernel";
                            }
                            kernel_fd = open(kernel_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                            if(kernel_fd == -1) 
                            {
                                fprintf(stderr, "disassemble: could not create kernel\n");
                                close(fd);
                                close(recipe_fd);
                                return 1;
                            }

                            write_to_recipe(RTYPE_KNN, kernel_filename, recipe_fd);
                            write(kernel_fd, kernel_data, hdr.kernel_size);
                            close(kernel_fd);
                            free(kernel_data);

                            write_to_recipe(RTYPE_RDA, &hdr.ramdisk_addr, recipe_fd);

                            ramdisk_data = malloc(hdr.ramdisk_size);
                            pread(fd, ramdisk_data, hdr.ramdisk_size, hdr.page_size + (kernel_pages * hdr.page_size));

                            if(ramdisk_data[0] == 0x1f && ramdisk_data[1] == 0x8b) 
                            {
                                ramdisk_filename = "ramdisk.gz";
                            } 
                            else 
                            {
                                ramdisk_filename = "ramdisk";
                            }
                            ramdisk_fd = open(ramdisk_filename, O_RDWR | O_CREAT | O_TRUNC, 0644);

                            if(ramdisk_fd == -1) 
                            {
                                fprintf(stderr, "disassemble: could not create ramdisk\n");
                                close(fd);
                                close(recipe_fd);
                                return 1;
                            }

                            write_to_recipe(RTYPE_RDN, ramdisk_filename, recipe_fd);
                            write(ramdisk_fd, ramdisk_data, hdr.ramdisk_size);
                            close(ramdisk_fd);
                            free(ramdisk_data);

                            if(hdr.second_size > 0) 
                            {
                                second_data = malloc(hdr.second_size);
                                second_pages = (hdr.second_size + hdr.page_size - 1) / hdr.page_size;
                                second_fd = open("second", O_RDWR | O_CREAT | O_TRUNC, 0644);

                                if(second_fd == -1) 
                                {
                                    fprintf(stderr, "disassemble: could not create second stage\n");
                                    free(second_data);
                                    close(fd);
                                    close(recipe_fd);
                                }

                                pread(fd, second_data, hdr.second_size, hdr.page_size + (kernel_pages * hdr.page_size)
                                                                                    + (ramdisk_pages * hdr.page_size));
                                write_to_recipe(RTYPE_SEN, "second", recipe_fd);
                                write(second_fd, second_data, hdr.second_size);
                                free(second_data);
                                close(second_fd);
                            }

                            write_to_recipe(RTYPE_SEA, &hdr.second_addr, recipe_fd);
                            write_to_recipe(RTYPE_OSV, &hdr.os_version, recipe_fd);
                            write_to_recipe(RTYPE_CMD, hdr.cmdline, recipe_fd);
                            write_to_recipe(RTYPE_PNA, hdr.name, recipe_fd);
                            write_to_recipe(RTYPE_IDV, hdr.id, recipe_fd);
                            write_to_recipe(RTYPE_ECM, hdr.extra_cmdline, recipe_fd);

                            if(hdr.header_version > 0) 
                            {
                                if(hdr.recovery_dtbo_size > 0) 
                                {
                                    recovery_pages = (hdr.recovery_dtbo_size + hdr.page_size - 1) / hdr.page_size;
                                }
                                write_to_recipe(RTYPE_REO, &hdr.recovery_dtbo_offset, recipe_fd);
                            }

                            if(hdr.header_version > 1) 
                            {
                                uint32_t dtb_pages = (hdr.dtb_size + hdr.page_size - 1) / hdr.page_size;
                                uint8_t  *dtb_data = malloc(hdr.dtb_size);
                                /* write_to_recipe(RTYPE_DTS, &hdr.dtb_size, recipe_fd); */
                                write_to_recipe(RTYPE_DTA, &hdr.dtb_addr, recipe_fd);
                                pread(fd, dtb_data, hdr.dtb_size, hdr.page_size + (kernel_pages * hdr.page_size)
                                                                                + (ramdisk_pages * hdr.page_size)
                                                                                + (second_pages * hdr.page_size)
                                                                                + (recovery_pages * hdr.page_size));
                                dtb_fd = open("dtb", O_RDWR | O_CREAT, 0644);
                                write(dtb_fd, dtb_data, hdr.dtb_size);
                                close(dtb_fd);
                                free(dtb_data);
                                write_to_recipe(RTYPE_DTN, "dtb", recipe_fd);
                            }

                            close(fd);
                            close(recipe_fd);
                        } 
                        else 
                        {
                            fprintf(stderr, "disassemble: could not create recipe.cfg\n");
                            close(fd);
                        }
                    } 
                    else 
                    {
                        fprintf(stderr, "disassemble: could not read header\n");
                        close(fd);
                        return 1;
                    }
                } 
                else 
                {
                    fprintf(stderr, "disassemble: could not open image %s\n", argv[2]);
                    return 1;
                }
            } 
            else 
            {
                fprintf(stderr, "Usage: bootimgtool disassemble <image>");
                return 1;
            }
        } 
        else 
        {
            fprintf(stderr, "Unknown operation: %s\n", argv[1]);
            return usage();
        }
    } 
    else 
    {
        return usage();
    }
    return 0;
}
