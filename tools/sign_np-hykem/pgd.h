/* SPDX-FileCopyrightText: 2013 tpu
   SPDX-FileCopyrightText: 2015 Hykem <hykem@hotmail.com>
   SPDX-License-Identifier: GPL-3.0-only */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libkirk/kirk_engine.h"
#include "libkirk/amctrl.h"
#include "utils.h"

typedef struct {
	unsigned char vkey[16];

	int open_flag;
	int key_index;
	int drm_type;
	int mac_type;
	int cipher_type;

	int data_size;
	int align_size;
	int block_size;
	int block_nr;
	int data_offset;
	int table_offset;

	unsigned char *buf;
} PGD_HEADER;

int encrypt_pgd(u8* data, int data_size, int block_size, int key_index, int drm_type, int flag, u8* key, u8* pgd_data);
int decrypt_pgd(u8* pgd_data, int pgd_size, int flag, u8* key);