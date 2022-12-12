// SPDX-FileCopyrightText: 2022 Linblow <dev@linblow.com>
// SPDX-License-Identifier: BSD-3-Clause
#ifndef LIBLIN_PSP_HDR_PBP_H
#define LIBLIN_PSP_HDR_PBP_H

#include <common/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Index of each file in the file_offset member. */
#define PBP_PARAM_SFO   (0)
#define PBP_ICON0_PNG   (1)
#define PBP_ICON1_PNG   (2) // same 2
#define PBP_ICON1_PMF   (2) // same 2
#define PBP_PIC0_PNG    (3)
#define PBP_PIC1_PNG    (4) // same 4
#define PBP_PICT1_PNG   (4) // same 4
#define PBP_SND0_AT3    (5)
#define PBP_DATA_PSP    (6)
#define PBP_DATA_PSAR   (7)

#define PBP_MAGIC       (0x00504250)
#define PBP_MAGIC_STR   "\0PBP"
#define PBP_MAX_FILES   (8)
#define PBP_HEADER_SIZE (0x28)

/**
 * PBP file data header.
 * A PBP file is a simple concatenation of pre-determined files.
 * All the structure members are Little-Endian.
 * 
 * The size of an embedded file data is determined as follows:
 *  size = file_offset[index + 1] - file_offset[index]
 * Where index is the file index whose size you want.
 * When the file index is 7:
 *  size = pbp_file_size - file_offset[7]
 * A size of zero means there is no file data.
 */
typedef struct {
    /** PBP file header magic "\0PBP". */
    u8 magic[4]; // 0x00
    /** File header version. Always 0x00010000 (1.0) or 0x00010001 (1.1). */ 
    u32 version; // 0x04
    union {
        /* Data offset by file name. */
        struct {
            /** PARAM.SFO file data offset. */
            u32 off_param;      // 0x08
            /** ICON0.PNG file data offset. */
            u32 off_icon0;      // 0x0c
            /** ICON1.PNG / ICON1.PMF file data offset. */
            u32 off_icon1;      // 0x10
            /** PIC0.PNG file data offset. */
            u32 off_pic0;       // 0x14
            /** PIC1.PNG / PICT1.PNG file data offset. */
            u32 off_pic1;       // 0x18
            /** SND0.AT3 file data offset. */
            u32 off_snd0;       // 0x1c
            /** DATA.PSP file data offset. */
            u32 off_data_psp;   // 0x20
            /** DATA.PSAR file data offset. */
            u32 off_data_psar;  // 0x24
        };
        /** File data offset by index. */
        u32 file_offset[8];
    };
    /* The embedded files data follows this structure. */
} ScePBPHeader; // size: 0x28

#ifdef __cplusplus
}
#endif

#endif /* LIBLIN_PSP_HDR_PBP_H */
