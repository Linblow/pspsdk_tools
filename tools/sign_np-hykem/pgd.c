// SPDX-FileCopyrightText: 2013 tpu
// SPDX-FileCopyrightText: 2015 Hykem <hykem@hotmail.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "pgd.h"

static u8 dnas_key1A90[] = {0xED, 0xE2, 0x5D, 0x2D, 0xBB, 0xF8, 0x12, 0xE5, 0x3C, 0x5C, 0x59, 0x32, 0xFA, 0xE3, 0xE2, 0x43};
static u8 dnas_key1AA0[] = {0x27, 0x74, 0xFB, 0xEB, 0xA4, 0xA0, 0x01, 0xD7, 0x02, 0x56, 0x9E, 0x33, 0x8C, 0x19, 0x57, 0x83};

/*
	PGD encrypt function.
*/
int encrypt_pgd(u8* data, int data_size, int block_size, int key_index, int drm_type, int flag, u8* key, u8* pgd_data)
{	
	MAC_KEY mkey;
	CIPHER_KEY ckey;

	// Additional size variables.
	int data_offset = 0x90;
	int align_size = (data_size + 15) &~ 15;
	int table_offset = data_offset + align_size;
	int block_nr = ((align_size + block_size - 1) &~ (block_size - 1)) / block_size;
	int pgd_size = 0x90 + align_size + block_nr * 16;
	
	// Build new PGD header.
	u8* pgd = (u8 *) malloc (pgd_size);
	memset(pgd, 0, pgd_size);
	memcpy(pgd + data_offset, data, data_size);

	// Set magic PGD.
	pgd[0] = 0x00;
	pgd[1] = 0x50;
	pgd[2] = 0x47;
	pgd[3] = 0x44;

	// Set key index and drm type.
	*(u32*)(pgd + 4) = key_index;
	*(u32*)(pgd + 8) = drm_type;
	
	// Select the hashing, crypto and open modes.
	int mac_type;
	int cipher_type;
	int open_flag = flag;
	if (drm_type == 1)
	{
		mac_type = 1;
		open_flag |= 4;
		if (key_index > 1)
		{
			mac_type = 3;
			open_flag |= 8;
		}
		cipher_type = 1;
	}
	else
	{
		mac_type = 2;
		cipher_type = 2;
	}
	
	// Select the fixed DNAS key.
	u8* fkey = NULL;
	if ((open_flag & 0x2) == 0x2)
		fkey = dnas_key1A90;
	if ((open_flag & 0x1) == 0x1)
		fkey = dnas_key1AA0;

	if (fkey == NULL)
	{
		printf("PGD: Invalid PGD DNAS flag! %08x\n", flag);
		return -1;
	}
	
	// Set the decryption parameters in the decrypted header.
	*(u32*)(pgd + 0x44) = data_size;
	*(u32*)(pgd + 0x48) = block_size;
	*(u32*)(pgd + 0x4C) = data_offset;
	
	// Generate random header and data keys.
	sceUtilsBufferCopyWithRange(pgd + 0x10, 0x30, 0, 0, KIRK_CMD_PRNG);
	
	// Encrypt the data.
	sceDrmBBCipherInit(&ckey, cipher_type, 2, pgd + 0x30, key, 0);
	sceDrmBBCipherUpdate(&ckey, pgd + data_offset, align_size);
	sceDrmBBCipherFinal(&ckey);
	
	// Build data MAC hash.
	int i;
	for (i = 0; i < block_nr; i++)
	{
		int rsize = align_size - i * block_size;
		if (rsize > block_size)
			rsize = block_size;

		sceDrmBBMacInit(&mkey, mac_type);
		sceDrmBBMacUpdate(&mkey, pgd + data_offset + i * block_size, rsize);
		sceDrmBBMacFinal(&mkey, pgd + table_offset + i * 16, key);
	}
	
	// Build table MAC hash.
	sceDrmBBMacInit(&mkey, mac_type);
	sceDrmBBMacUpdate(&mkey, pgd + table_offset, block_nr * 16);
	sceDrmBBMacFinal(&mkey, pgd + 0x60, key);
	
	// Encrypt the PGD header block (0x30 bytes).
	sceDrmBBCipherInit(&ckey, cipher_type, 2, pgd + 0x10, key, 0);
	sceDrmBBCipherUpdate(&ckey, pgd + 0x30, 0x30);
	sceDrmBBCipherFinal(&ckey);
	
	// Build MAC hash at 0x70 (key hash).
	sceDrmBBMacInit(&mkey, mac_type);
	sceDrmBBMacUpdate(&mkey, pgd + 0x00, 0x70);
	sceDrmBBMacFinal(&mkey, pgd + 0x70, key);

	// Build MAC hash at 0x80 (DNAS hash).
	sceDrmBBMacInit(&mkey, mac_type);
	sceDrmBBMacUpdate(&mkey, pgd + 0x00, 0x80);
	sceDrmBBMacFinal(&mkey, pgd + 0x80, fkey);
	
	// Copy back the generated PGD file.
	memcpy(pgd_data, pgd, pgd_size);

	return pgd_size;
}

/*
	PGD decrypt function.
*/
int decrypt_pgd(u8* pgd_data, int pgd_size, int flag, u8* key)
{
	int result;
	PGD_HEADER pgd;
	MAC_KEY mkey;
	CIPHER_KEY ckey;
	u8* fkey;

	// Read in the PGD header parameters.
	memset(&pgd, 0, sizeof(PGD_HEADER));

	pgd.buf = pgd_data;
	pgd.key_index = *(u32*)(pgd_data + 4);
	pgd.drm_type  = *(u32*)(pgd_data + 8);

	// Set the hashing, crypto and open modes.
	if (pgd.drm_type == 1)
	{
		pgd.mac_type = 1;
		flag |= 4;

		if(pgd.key_index > 1)
		{
			pgd.mac_type = 3;
			flag |= 8;
		}
		pgd.cipher_type = 1;
	}
	else
	{
		pgd.mac_type = 2;
		pgd.cipher_type = 2;
	}
	pgd.open_flag = flag;

	// Get the fixed DNAS key.
	fkey = NULL;
	if ((flag & 0x2) == 0x2)
		fkey = dnas_key1A90;
	if ((flag & 0x1) == 0x1)
		fkey = dnas_key1AA0;

	if (fkey == NULL)
	{
		printf("PGD: Invalid PGD DNAS flag! %08x\n", flag);
		return -1;
	}

	// Test MAC hash at 0x80 (DNAS hash).
	sceDrmBBMacInit(&mkey, pgd.mac_type);
	sceDrmBBMacUpdate(&mkey, pgd_data, 0x80);
	result = sceDrmBBMacFinal2(&mkey, pgd_data + 0x80, fkey);

	if (result)
	{
		printf("PGD: Invalid PGD 0x80 MAC hash!\n");
		return -1;
	}

	// Test MAC hash at 0x70 (key hash).
	sceDrmBBMacInit(&mkey, pgd.mac_type);
	sceDrmBBMacUpdate(&mkey, pgd_data, 0x70);

	// If a key was provided, check it against MAC 0x70.
	if (!isEmpty(key, 0x10))
	{
		result = sceDrmBBMacFinal2(&mkey, pgd_data + 0x70, key);
		if (result)
		{
			printf("PGD: Invalid PGD 0x70 MAC hash!\n");
			return -1;
		}
		else
		{
			memcpy(pgd.vkey, key, 16);
		}
	}
	else
	{
		// Generate the key from MAC 0x70.
		bbmac_getkey(&mkey, pgd_data + 0x70, pgd.vkey);
	}

	// Decrypt the PGD header block (0x30 bytes).
	sceDrmBBCipherInit(&ckey, pgd.cipher_type, 2, pgd_data + 0x10, pgd.vkey, 0);
	sceDrmBBCipherUpdate(&ckey, pgd_data + 0x30, 0x30);
	sceDrmBBCipherFinal(&ckey);

	// Get the decryption parameters from the decrypted header.
	pgd.data_size   = *(u32*)(pgd_data + 0x44);
	pgd.block_size  = *(u32*)(pgd_data + 0x48);
	pgd.data_offset = *(u32*)(pgd_data + 0x4c);

	// Additional size variables.
	pgd.align_size = (pgd.data_size + 15) &~ 15;
	pgd.table_offset = pgd.data_offset + pgd.align_size;
	pgd.block_nr = (pgd.align_size + pgd.block_size - 1) &~ (pgd.block_size - 1);
	pgd.block_nr = pgd.block_nr / pgd.block_size;

	if ((pgd.align_size + pgd.block_nr * 16) > pgd_size)
	{
		printf("ERROR: Invalid PGD data size!\n");
		return -1;
	}

	// Test MAC hash at 0x60 (table hash).
	sceDrmBBMacInit(&mkey, pgd.mac_type);
	sceDrmBBMacUpdate(&mkey, pgd_data + pgd.table_offset, pgd.block_nr * 16);
	result = sceDrmBBMacFinal2(&mkey, pgd_data + 0x60, pgd.vkey);

	if (result)
	{
		printf("ERROR: Invalid PGD 0x60 MAC hash!\n");
		return -1;
	}

	// Decrypt the data.
	sceDrmBBCipherInit(&ckey, pgd.cipher_type, 2, pgd_data + 0x30, pgd.vkey, 0);
	sceDrmBBCipherUpdate(&ckey, pgd_data + 0x90, pgd.align_size);
	sceDrmBBCipherFinal(&ckey);

	return pgd.data_size;
}