/* SPDX-FileCopyrightText: 2013 tpu
   SPDX-FileCopyrightText: 2015 Hykem <hykem@hotmail.com>
   SPDX-License-Identifier: GPL-3.0-only */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libkirk/kirk_engine.h"
#include "libkirk/psp_headers.h"
#include "utils.h"

typedef struct {
	u32 tag;
	u8  key[16];
	u32 code;
	u32 type;
} TAG_KEY;

int sign_eboot(u8 *eboot, int eboot_size, int tag, u8 *seboot);
