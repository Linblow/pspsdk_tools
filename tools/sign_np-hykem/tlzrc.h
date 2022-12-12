/* SPDX-FileCopyrightText: 2013 tpu
   SPDX-FileCopyrightText: 2015 Hykem <hykem@hotmail.com>
   SPDX-License-Identifier: GPL-3.0-only */

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "utils.h"

typedef struct {
	u8 *input;
	int in_ptr;
	int in_len;

	u8 *output;
	int out_ptr;
	int out_len;

	u32 range;
	u32 code;
	u32 out_code;
	u8 lc;

	u8 bm_literal[8][256];
	u8 bm_dist_bits[8][39];
	u8 bm_dist[18][8];
	u8 bm_match[8][8];
	u8 bm_len[8][31];
} LZRC_DECODE;

int lzrc_compress(void *out, int out_len, void *in, int in_len);
int lzrc_decompress(void *out, int out_len, void *in, int in_len);