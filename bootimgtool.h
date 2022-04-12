#include "bootimg.h"

enum rtypes {
    RTYPE_KNN,        /* "knn" - kernel name */
    RTYPE_KNS,        /* "kns" - kernel size */
    RTYPE_KNA,        /* "kna" - kernel address */
    RTYPE_RDN,        /* "rdn" - ramdisk name */
    RTYPE_RDS,        /* "rds" - ramdisk size */
    RTYPE_RDA,        /* "rda" - ramdisk address */
    RTYPE_SEN,        /* "sen" - second filename */
    RTYPE_SES,        /* "ses" - second size */
    RTYPE_SEA,        /* "sea" - second address */
    RTYPE_TAA,        /* "taa" - tags address */
    RTYPE_PAS,        /* "pas" - page size */
    RTYPE_HEV,        /* "hev" - header version */
    RTYPE_OSV,        /* "osv" - os version */
    RTYPE_PNA,        /* "pna" - product name */
    RTYPE_CMD,        /* "cmd" - cmdline */
    RTYPE_IDV,        /* "idv" - id */
    RTYPE_ECM,        /* "ecm" - extra cmdline */
    RTYPE_RES,        /* "res" - recovery dtbo image size */
    RTYPE_REO,        /* "reo" - recovery dtbo image offset */
    RTYPE_DTS,        /* "dts" - DTB size */
    RTYPE_DTA,        /* "dto" - DTB addr */
    RTYPE_DTN,        /* "dtn" - DTB filename */
    RTYPE_RESERVED
};

int   is_valid_image(int fd);
char* get_os_patch_level(uint32_t os_patch_level);
char* get_os_version(uint32_t os_version);
int   read_header(int fd, struct bootimg_hdr_0_2 *header);
void  show_info(struct bootimg_hdr_0_2 *header);
int   usage();
void  write_to_recipe(enum rtypes type, void *value, int fd);