// SPDX-FileCopyrightText: 2015 Hykem <hykem@hotmail.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "sign_np.h"

u8 *load_file_from_ISO(const char *iso, char *name, int *size)
{
	int ret;
	u32 lba;
	u8 *buf;

	ret = isoOpen(iso);
	if (ret < 0) {
		return NULL;
	}

	ret = isoGetFileInfo(name, (u32*)size, &lba);
	if (ret < 0) {
		isoClose();
		return NULL;
	}

	buf = malloc(*size);
	if (buf == NULL) {
		isoClose();
		return NULL;
	}

	ret = isoRead(buf, lba, 0, *size);
	if (ret < 0) {
		isoClose();
		return NULL;
	}

	isoClose();
	return buf;
}

int sfo_get_key(u8 *sfo_buf, char *name, void *value)
{
	u32 i, offset;
	SFO_Header *sfo = (SFO_Header*)sfo_buf;
	SFO_Entry *sfo_keys = (SFO_Entry*)(sfo_buf + 0x14);

	if (sfo->magic != PSF_MAGIC)
		return -1;

	for (i = 0; i < sfo->key_count; i++)
	{
		offset = sfo_keys[i].name_offset;
		offset += sfo->key_offset;
		
		if (strcmp((char*)sfo_buf + offset, name) == 0)
		{
			offset = sfo_keys[i].data_offset;
			offset += sfo->val_offset;
			memcpy(value, sfo_buf + offset, sfo_keys[i].val_size);
			return sfo_keys[i].val_size;
		}
	}

	return -1;
}

int sfo_put_key(u8 *sfo_buf, char *name, void *value)
{
	u32 i, offset;
	SFO_Header *sfo = (SFO_Header*)sfo_buf;
	SFO_Entry *sfo_keys = (SFO_Entry*)(sfo_buf + 0x14);

	if (sfo->magic != PSF_MAGIC)
		return -1;

	for (i = 0; i < sfo->key_count; i++)
	{
		offset = sfo_keys[i].name_offset;
		offset += sfo->key_offset;
		
		if (strcmp((char*)sfo_buf + offset, name) == 0)
		{
			offset = sfo_keys[i].data_offset;
			offset += sfo->val_offset;
			memcpy(sfo_buf + offset, value, sfo_keys[i].val_size);
			return 0;
		}
	}

	return -1;
}

void encrypt_table(u8 *table)
{
	u32 *p = (u32*)table;
	u32 k0, k1, k2, k3;

	k0 = p[0]^p[1];
	k1 = p[1]^p[2];
	k2 = p[0]^p[3];
	k3 = p[2]^p[3];

	p[4] ^= k3;
	p[5] ^= k1;
	p[6] ^= k2;
	p[7] ^= k0;
}

NPUMDIMG_HEADER* forge_npumdimg(int iso_size, int iso_blocks, int block_basis, char *content_id, int np_flags, u8 *version_key, u8 *header_key, u8 *data_key)
{
	// Build NPUMDIMG header.
	NPUMDIMG_HEADER *np_header = (NPUMDIMG_HEADER *) malloc (sizeof(NPUMDIMG_HEADER));
	memset(np_header, 0, sizeof(NPUMDIMG_HEADER));
	
	// Set magic NPUMDIMG.
	np_header->magic[0] = 0x4E;
	np_header->magic[1] = 0x50;
	np_header->magic[2] = 0x55;
	np_header->magic[3] = 0x4D;
	np_header->magic[4] = 0x44;
	np_header->magic[5] = 0x49;
	np_header->magic[6] = 0x4D;
	np_header->magic[7] = 0x47;
	
	// Set flags and block basis.
	np_header->np_flags = np_flags;
	np_header->block_basis = block_basis;
	
	// Set content ID.
	memcpy(np_header->content_id, content_id, strlen(content_id));
	
	// Set inner body parameters.
	np_header->body.sector_size = 0x800;

	if (iso_size > 0x40000000) 
		np_header->body.unk_2 = 0xE001;
	else 
		np_header->body.unk_2 = 0xE000;
	
	np_header->body.unk_4 = 0x0;
	np_header->body.unk_8 = 0x1010;
	np_header->body.unk_12 = 0x0;
	np_header->body.unk_16 = 0x0;
	np_header->body.lba_start = 0x0;
	np_header->body.unk_24 = 0x0;
	
	if(((iso_blocks * block_basis) - 1) > 0x6C0BF)
		np_header->body.nsectors = 0x6C0BF;
	else
		np_header->body.nsectors = (iso_blocks * block_basis) - 1;
		
	np_header->body.unk_32 = 0x0;
	np_header->body.lba_end = (iso_blocks * block_basis) - 1;
	np_header->body.unk_40 = 0x01003FFE;
	np_header->body.block_entry_offset = 0x100;
	
	memcpy(np_header->body.disc_id, content_id + 7, 4);
	np_header->body.disc_id[4] = '-';
	memcpy(np_header->body.disc_id + 5, content_id + 11, 5);
	
	np_header->body.header_start_offset = 0x0;
	np_header->body.unk_68 = 0x0;
	np_header->body.unk_72 = 0x0;
	np_header->body.bbmac_param = 0x0;
	
	np_header->body.unk_74 = 0x0;
	np_header->body.unk_75 = 0x0;
	np_header->body.unk_76 = 0x0;
	np_header->body.unk_80 = 0x0;
	np_header->body.unk_84 = 0x0;
	np_header->body.unk_88 = 0x0;
	np_header->body.unk_92 = 0x0;
	
	// Set keys.
	memset(np_header->header_key, 0, 0x10);
	memset(np_header->data_key, 0, 0x10);
	memset(np_header->header_hash, 0, 0x10);
	memset(np_header->padding, 0, 0x8);
	
	// Copy header and data keys.
	memcpy(np_header->header_key, header_key, 0x10);
	memcpy(np_header->data_key, data_key, 0x10);

	// Generate random padding.
	sceUtilsBufferCopyWithRange(np_header->padding, 0x8, 0, 0, KIRK_CMD_PRNG);
	
	// Prepare buffers to encrypt the NPUMDIMG body.
	MAC_KEY mck;
	CIPHER_KEY bck;
	
	// Encrypt NPUMDIMG body.
	sceDrmBBCipherInit(&bck, 1, 2, np_header->header_key, version_key, 0);
	sceDrmBBCipherUpdate(&bck, (u8 *)(np_header) + 0x40, 0x60);
	sceDrmBBCipherFinal(&bck);
	
	// Generate header hash.
	sceDrmBBMacInit(&mck, 3);
	sceDrmBBMacUpdate(&mck, (u8 *)np_header, 0xC0);
	sceDrmBBMacFinal(&mck, np_header->header_hash, version_key);
	bbmac_build_final2(3, np_header->header_hash);
	
	// Prepare the signature hash input buffer.
	u8 npumdimg_sha1_inbuf[0xD8 + 0x4];
	u8 npumdimg_sha1_outbuf[0x14];
	memset(npumdimg_sha1_inbuf, 0, 0xD8 + 0x4);
	memset(npumdimg_sha1_outbuf, 0, 0x14);
	
	// Set SHA1 data size.
	npumdimg_sha1_inbuf[0] = 0xD8;
	memcpy(npumdimg_sha1_inbuf + 0x4, (u8 *)np_header, 0xD8);
	
	// Hash the input buffer.
	if (sceUtilsBufferCopyWithRange(npumdimg_sha1_outbuf, 0x14, npumdimg_sha1_inbuf, 0xD8 + 0x4, KIRK_CMD_SHA1_HASH) != 0)
	{
		fprintf(stderr, "ERROR: Failed to generate SHA1 hash for NPUMDIMG header!\n");
		return NULL;
	}
	
	// Prepare ECDSA signature buffer.
	u8 npumdimg_sign_buf_in[0x34];
	u8 npumdimg_sign_buf_out[0x28];
	memset(npumdimg_sign_buf_in, 0, 0x34);
	memset(npumdimg_sign_buf_out, 0, 0x28);
	
	// Create ECDSA key pair.
	u8 npumdimg_keypair[0x3C];
	memcpy(npumdimg_keypair, npumdimg_private_key, 0x14);
	memcpy(npumdimg_keypair + 0x14, npumdimg_public_key, 0x28);
		
	// Encrypt NPUMDIMG private key.
	u8 npumdimg_private_key_enc[0x20];
	memset(npumdimg_private_key_enc, 0, 0x20);
	encrypt_kirk16_private(npumdimg_private_key_enc, npumdimg_keypair);
	
	// Generate ECDSA signature.
	memcpy(npumdimg_sign_buf_in, npumdimg_private_key_enc, 0x20);
	memcpy(npumdimg_sign_buf_in + 0x20, npumdimg_sha1_outbuf, 0x14);
	if (sceUtilsBufferCopyWithRange(npumdimg_sign_buf_out, 0x28, npumdimg_sign_buf_in, 0x34, KIRK_CMD_ECDSA_SIGN) != 0)
	{
		fprintf(stderr, "ERROR: Failed to generate ECDSA signature for NPUMDIMG header!\n");
		return NULL;
	}
	
	// Verify the generated ECDSA signature.
	u8 test_npumdimg_sign[0x64];
	memcpy(test_npumdimg_sign, npumdimg_public_key, 0x28);
    memcpy(test_npumdimg_sign + 0x28, npumdimg_sha1_outbuf, 0x14);
    memcpy(test_npumdimg_sign + 0x3C, npumdimg_sign_buf_out, 0x28);
    if (sceUtilsBufferCopyWithRange(0, 0, test_npumdimg_sign, 0x64, KIRK_CMD_ECDSA_VERIFY) != 0)
	{
		fprintf(stderr, "ERROR: ECDSA signature for NPUMDIMG header is invalid!\n");
		return NULL;
	}
	else
	{
		// printf("ECDSA signature for NPUMDIMG header is valid!\n");
	}

	// Store the signature.
	memcpy(np_header->ecdsa_sig, npumdimg_sign_buf_out, 0x28);
	
	return np_header;
}

int write_pbp(FILE *f, char *iso_name, char *content_id, int np_flags, u8 *startdat_buf, int startdat_size, u8 *pgd_buf, int pgd_size)
{
	// Get all data files.
	int param_sfo_size = 0;
	int icon0_size = 0;
	int icon1_size = 0;
	int pic0_size = 0;
	int pic1_size = 0;
	int snd0_size = 0;
	u8 *param_sfo_buf = load_file_from_ISO(iso_name, "/PSP_GAME/PARAM.SFO", &param_sfo_size);
	u8 *icon0_buf = load_file_from_ISO(iso_name, "/PSP_GAME/ICON0.PNG", &icon0_size);
	u8 *icon1_buf = load_file_from_ISO(iso_name, "/PSP_GAME/ICON1.PMF",&icon1_size);
	u8 *pic0_buf = load_file_from_ISO(iso_name, "/PSP_GAME/PIC0.PNG", &pic0_size);
	u8 *pic1_buf = load_file_from_ISO(iso_name, "/PSP_GAME/PIC1.PNG", &pic1_size);
	u8 *snd0_buf = load_file_from_ISO(iso_name, "/PSP_GAME/SND0.AT3", &snd0_size);
	
	// Get system version from PARAM.SFO.
	u8 sys_ver[0x4];
	memset(sys_ver, 0, 0x4);
	sfo_get_key(param_sfo_buf, "PSP_SYSTEM_VER", sys_ver);
	// printf("PSP_SYSTEM_VER: %s\n\n", sys_ver);
	
	// Change disc ID in PARAM.SFO.
	u8 disc_id[0x10];
	memset(disc_id, 0, 0x10);
	memcpy(disc_id, content_id + 0x7, 0x9);
	sfo_put_key(param_sfo_buf, "DISC_ID", disc_id);
	
	// Change category in PARAM.SFO.
	sfo_put_key(param_sfo_buf, "CATEGORY", "EG");
	
	// Build DATA.PSP (content ID + flags).
	// printf("Building DATA.PSP...\n");
	int data_psp_size = 0x594 + ((startdat_size) ? startdat_size + 0xC : 0) + pgd_size;
	u8 *data_psp_buf = (u8 *) malloc (data_psp_size);
	memset(data_psp_buf, 0, data_psp_size);
	memcpy(data_psp_buf + 0x560, content_id, strlen(content_id));
	*(u32 *)(data_psp_buf + 0x590) = se32(np_flags);
	
	// DATA.PSP contains PARAM.SFO signature.
	u8 *data_psp_param_buf = (u8 *) malloc (param_sfo_size + 0x30);
	memset(data_psp_param_buf, 0, param_sfo_size + 0x30);
	memcpy(data_psp_param_buf, param_sfo_buf, param_sfo_size);
	memcpy(data_psp_param_buf + param_sfo_size, data_psp_buf + 0x560, 0x30);
	
	// Prepare the signature hash input buffer.
	u8 *data_psp_sha1_inbuf = (u8 *) malloc (param_sfo_size + 0x30 + 0x4);
	u8 data_psp_sha1_outbuf[0x14];
	memset(data_psp_sha1_inbuf, 0, param_sfo_size + 0x30 + 0x4);
	memset(data_psp_sha1_outbuf, 0, 0x14);
	
	// Set SHA1 data size.
	*(u32 *)data_psp_sha1_inbuf = param_sfo_size + 0x30;
	memcpy(data_psp_sha1_inbuf + 0x4, (u8*)data_psp_param_buf, param_sfo_size + 0x30);
	
	// Hash the input buffer.
	if (sceUtilsBufferCopyWithRange(data_psp_sha1_outbuf, 0x14, data_psp_sha1_inbuf, param_sfo_size + 0x30 + 0x4, KIRK_CMD_SHA1_HASH) != 0)
	{
		fprintf(stderr, "ERROR: Failed to generate SHA1 hash for DATA.PSP!\n");
		return 0;
	}
	
	// Prepare ECDSA signature buffer.
	u8 data_psp_sign_buf_in[0x34];
	u8 data_psp_sign_buf_out[0x28];
	memset(data_psp_sign_buf_in, 0, 0x34);
	memset(data_psp_sign_buf_out, 0, 0x28);
	
	// Create ECDSA key pair.
	u8 data_psp_keypair[0x3C];
	memcpy(data_psp_keypair, npumdimg_private_key, 0x14);
	memcpy(data_psp_keypair + 0x14, npumdimg_public_key, 0x28);
	
	// Encrypt NPUMDIMG private key.
	u8 data_psp_private_key_enc[0x20];
	memset(data_psp_private_key_enc, 0, 0x20);
	encrypt_kirk16_private(data_psp_private_key_enc, data_psp_keypair);
	
	// Generate ECDSA signature.
	memcpy(data_psp_sign_buf_in, data_psp_private_key_enc, 0x20);
	memcpy(data_psp_sign_buf_in + 0x20, data_psp_sha1_outbuf, 0x14);
	if (sceUtilsBufferCopyWithRange(data_psp_sign_buf_out, 0x28, data_psp_sign_buf_in, 0x34, KIRK_CMD_ECDSA_SIGN) != 0)
	{
		fprintf(stderr, "ERROR: Failed to generate ECDSA signature for DATA.PSP!\n");
		return 0;
	}
	
	// Verify the generated ECDSA signature.
	u8 test_data_psp_sign[0x64];
	memcpy(test_data_psp_sign, npumdimg_public_key, 0x28);
    memcpy(test_data_psp_sign + 0x28, data_psp_sha1_outbuf, 0x14);
    memcpy(test_data_psp_sign + 0x3C, data_psp_sign_buf_out, 0x28);
    if (sceUtilsBufferCopyWithRange(0, 0, test_data_psp_sign, 0x64, KIRK_CMD_ECDSA_VERIFY) != 0)
	{
		fprintf(stderr, "ERROR: ECDSA signature for DATA.PSP is invalid!\n");
		return 0;
	}
	else
	{
		// printf("ECDSA signature for DATA.PSP is valid!\n");
	}

	// Store the signature.
	memcpy(data_psp_buf, data_psp_sign_buf_out, 0x28);
	
	// Append STARTDAT file to DATA.PSP, if provided.
	if (startdat_size)
	{
		int startdat_offset = 0x594 + 0xC;
		memcpy(data_psp_buf + startdat_offset, startdat_buf, startdat_size);
		
		free(startdat_buf);
	}
	
	// Append encrypted OPNSSMP file to DATA.PSP, if provided.
	if (pgd_size)
	{
		int pgd_offset = (startdat_size) ? (0x594 + 0xC + startdat_size) : 0x594;
		memcpy(data_psp_buf + pgd_offset, pgd_buf, pgd_size);
		
		// Store OPNSSMP offset and size.
		*(u32 *)(data_psp_buf + 0x28 + 0x8) = pgd_offset;
		*(u32 *)(data_psp_buf + 0x28 + 0x8 + 0x4) = pgd_size;
		
		free(pgd_buf);
	}
	
	// Build empty DATA.PSAR.
	int data_psar_size = 0x100;
	u8 *data_psar_buf = (u8 *) malloc (data_psar_size);	
	memset(data_psar_buf, 0, data_psar_size);

	// Calculate header size.
	int header_size = icon0_size + icon1_size + pic0_size + pic1_size + snd0_size + param_sfo_size + data_psp_size;

	// Allocate PBP header.
	u8 *pbp_header = malloc(header_size + 4096);
	memset(pbp_header, 0, header_size + 4096);

	// Write magic.
	*(u32*)(pbp_header + 0) = 0x50425000;
	*(u32*)(pbp_header + 4) = 0x00010001;

	// Set header offset.
	int header_offset = 0x28;

	*(u32*)(pbp_header + 0x08) = header_offset;
	memcpy(pbp_header + header_offset, param_sfo_buf, param_sfo_size);
	header_offset += param_sfo_size;

	*(u32*)(pbp_header + 0x0C) = header_offset;
	memcpy(pbp_header + header_offset, icon0_buf, icon0_size);
	header_offset += icon0_size;
	
	*(u32*)(pbp_header + 0x10) = header_offset;
	memcpy(pbp_header + header_offset, icon1_buf, icon1_size);
	header_offset += icon1_size;
	
	*(u32*)(pbp_header + 0x14) = header_offset;
	memcpy(pbp_header + header_offset, pic0_buf, pic0_size);
	header_offset += pic0_size;
	
	*(u32*)(pbp_header + 0x18) = header_offset;
	memcpy(pbp_header + header_offset, pic1_buf, pic1_size);
	header_offset += pic1_size;
	
	*(u32*)(pbp_header + 0x1C) = header_offset;
	memcpy(pbp_header + header_offset, snd0_buf, snd0_size);
	header_offset += snd0_size;

	*(u32*)(pbp_header + 0x20) = header_offset;
	memcpy(pbp_header + header_offset, data_psp_buf, data_psp_size);
	header_offset += data_psp_size;
	
	// DATA.PSAR is 0x100 aligned.
	header_offset = (header_offset + 15) &~ 15;
	while (header_offset % 0x100) 
		header_offset += 0x10;

	*(u32*)(pbp_header + 0x24) = header_offset;
	memcpy(pbp_header + header_offset, data_psar_buf, data_psar_size);
	header_offset += data_psar_size;

	// Write PBP.
	fwrite(pbp_header, header_offset, 1, f);
	
	// Clean up.
	free(data_psar_buf);
	free(data_psp_buf);
	free(data_psp_param_buf);
	free(data_psp_sha1_inbuf);
	free(pbp_header);

	return header_offset;
}

//TODO add option -v / --verbose for the commented printf statements
void print_usage()
{
	printf("psp-sign-np v1.0.4 by Hykem\n"
	       "Convert PSP ISOs to signed PSN PBPs.\n\n"
	       "Usage: psp-sign-np -pbp [-c] <input> <output> <cid> <key> [<startdat> [<opnssmp>]]\n"
	       "       psp-sign-np -elf <input> <output> <tag>\n"
	       "\n"
	       "- Modes:\n"
	       "[-pbp]: Encrypt and sign a PSP ISO into a PSN EBOOT.PBP\n"
	       "[-elf]: Encrypt and sign a ELF file into an EBOOT.BIN\n"
	       "\n"
	       "- PBP mode:\n"
	       "[-c]: Compress data.\n"
	       "<input>: A valid PSP ISO image with a signed EBOOT.BIN\n"
	       "<output>: Resulting signed EBOOT.PBP file\n"
	       "<cid>: Content ID (XXYYYY-AAAABBBBB_CC-DDDDDDDDDDDDDDDD)\n"
	       "<key>: Version key (16 bytes) or Fixed Key (0)\n"
	       "<startdat>: PNG image to be used as boot screen (optional)\n"
	       "<opnssmp>: OPNSSMP.BIN module (optional)\n"
	       "\n"
	       "- ELF mode:\n"
	       "<input>: A valid ELF file\n"
	       "<output>: Resulting signed EBOOT.BIN file\n"
	       "<tag>: 00 - 0x8004FD03  14 - 0xD91617F0\n"
           "       01 - 0xD91605F0  15 - 0xD91618F0\n"
           "       02 - 0xD91606F0  16 - 0xD91619F0\n"
           "       03 - 0xD91608F0  17 - 0xD9161AF0\n"
           "       04 - 0xD91609F0  18 - 0xD9161EF0\n"
           "       05 - 0xD9160AF0  19 - 0xD91620F0\n"
           "       06 - 0xD9160BF0  20 - 0xD91621F0\n"
           "       07 - 0xD91610F0  21 - 0xD91622F0\n"
           "       08 - 0xD91611F0  22 - 0xD91623F0\n"
           "       09 - 0xD91612F0  23 - 0xD91624F0\n"
           "       10 - 0xD91613F0  24 - 0xD91628F0\n"
           "       11 - 0xD91614F0  25 - 0xD91680F0\n"
           "       12 - 0xD91615F0  26 - 0xD91681F0\n"
           "       13 - 0xD91616F0  27 - 0xD91690F0\n");
}

int main(int argc, char *argv[])
{
	if ((argc <= 1) || (argc > 9))
	{
		print_usage();
		return 0;
	}
	
	// Keep track of each argument's offset.
	int arg_offset = 0;
	
	// ELF signing mode.
	if (!strcmp(argv[arg_offset + 1], "-elf") && (argc > (arg_offset + 4)))
	{
		// Skip the mode argument.
		arg_offset++;
		
		// Open files.
		char *elf_name = argv[arg_offset + 1];
		char *bin_name = argv[arg_offset + 2];
		int tag = atoi(argv[arg_offset + 3]);
		FILE* elf = fopen(elf_name, "rb");
		FILE* bin = fopen(bin_name, "wb");
		
		// Check input file.
		if (elf == NULL)
		{
			fprintf(stderr, "ERROR: Please check your input file!\n");
			fclose(elf);
			fclose(bin);
			return 0;
		}
		
		// Check output file.
		if (bin == NULL)
		{
			fprintf(stderr, "ERROR: Please check your output file!\n");
			fclose(elf);
			fclose(bin);
			return 0;
		}
		
		// Check tag.
		if ((tag < 0) || (tag > 27))
		{
			fprintf(stderr, "ERROR: Invalid EBOOT tag!\n");
			fclose(elf);
			fclose(bin);
			return 0;
		}
		
		// Get ELF size.
		fseek(elf, 0, SEEK_END);
		int elf_size = ftell(elf);
		fseek(elf, 0, SEEK_SET);
		
		kirk_init();
	
		// Read ELF file.
		u8 *elf_buf = (u8 *) malloc (elf_size);
		if ((int)fread(elf_buf, 1, elf_size, elf) != elf_size)
		{
			fprintf(stderr, "ERROR: Cannot read ELF file\n");
			fclose(elf);
			fclose(bin);
			return 0;
		}
		
		// Sign the ELF file.
		u8 *seboot_buf = (u8 *) malloc (elf_size + 4096);
		memset(seboot_buf, 0, elf_size + 4096);
		int seboot_size = sign_eboot(elf_buf, elf_size, tag, seboot_buf);
		
		// Exit in case of error.
		if (seboot_size < 0)
		{
			fclose(elf);
			fclose(bin);
			return 0;
		}
		
		// Write the signed EBOOT.BIN file.
		fwrite(seboot_buf, seboot_size, 1, bin);
		
		// Clean up.
		fclose(bin);
		fclose(elf);
		free(seboot_buf);
		free(elf_buf);
		
		return 0;
	}
	else if (!strcmp(argv[arg_offset + 1], "-pbp") && (argc > (arg_offset + 5)))  // EBOOT signing mode.
	{
		// Skip the mode argument.
		arg_offset++;
		
		// Check if the data must be compressed.
		int compress = 0;
		if (!strcmp(argv[arg_offset + 1], "-c"))
		{
			compress = 1;
			arg_offset++;
		}
		
		// Check for enough arguments after the compression flag.
		if (argc < (arg_offset + 5))
		{
			print_usage();
			return 0;
		}
		
		// Open files.
		char *iso_name = argv[arg_offset + 1];
		char *pbp_name = argv[arg_offset + 2];
		FILE* iso = fopen(iso_name, "rb");
		FILE* pbp = fopen(pbp_name, "wb");
		
		// Get Content ID from input.
		char *cid = argv[arg_offset + 3];
		char content_id[0x30];
		memset(content_id, 0, 0x30);
		memcpy(content_id, cid, strlen(cid));
		
		// Set version, header and data keys.
		int use_version_key = 0;
		u8 version_key[0x10];
		u8 header_key[0x10];
		u8 data_key[0x10];
		memset(version_key, 0, 0x10);
		memset(header_key, 0, 0x10);
		memset(data_key, 0, 0x10);
		
		// Read version key from input.
		char *vk = argv[arg_offset + 4];
		if (is_hex(vk, 0x20))
		{
			unsigned char user_key[0x10];
			hex_to_bytes(user_key, vk, 0x20);
			memcpy(version_key, user_key, 0x10);
			use_version_key = 1;
		}		
		
		// Check input file.
		if (iso == NULL)
		{
			fprintf(stderr, "ERROR: Please check your input file!\n");
			fclose(iso);
			fclose(pbp);
			return 0;
		}
		
		// Check output file.
		if (pbp == NULL)
		{
			fprintf(stderr, "ERROR: Please check your output file!\n");
			fclose(iso);
			fclose(pbp);
			return 0;
		}
			
		// Get ISO size.
		fseeko64(iso, 0, SEEK_END);
		long long iso_size = ftello64(iso);
		fseeko64(iso, 0, SEEK_SET);
		
		kirk_init();
		
		// Set keys' context.
		MAC_KEY mkey;
		CIPHER_KEY ckey;
			
		// Set flags and block size data.
		int np_flags = (use_version_key) ? 0x2 : (0x3 | (0x01000000));
		int block_basis = 0x10;
		int block_size = block_basis * 2048;
		long long iso_blocks = (iso_size + block_size - 1) / block_size;
		
		// Generate random header key.
		sceUtilsBufferCopyWithRange(header_key, 0x10, 0, 0, KIRK_CMD_PRNG);
		
		// Generate fixed key, if necessary.
		if (!use_version_key)
			sceNpDrmGetFixedKey(version_key, content_id, np_flags);
		
		// Check for optional files.
		char *startdat_name = NULL;
		char *opnssmp_name = NULL;
		
		if (argc > (arg_offset + 5))
		{
			char *ex_file_name1 = argv[arg_offset + 5];
			char *ex_file_name2 = argv[arg_offset + 6];
			char png_magic[4] = {0x89, 0x50, 0x4E, 0x47};  // %PNG
			char psp_magic[4] = {0x7E, 0x50, 0x53, 0x50};  // ~PSP
		
			// Check the first optional file.
			if (ex_file_name1)
			{
				// Read the first optional file's header.
				char ex_file1_magic[4] = {0x00, 0x00, 0x00, 0x00};
				FILE* ex_file1 = fopen(ex_file_name1, "rb");
			
				if (ex_file1 != NULL && fread(ex_file1_magic, 1, 4, ex_file1) != 4)
				{
					fprintf(stderr, "ERROR: Cannot read optional file 1\n");
					fclose(ex_file1);
					fclose(iso);
					fclose(pbp);
					return 0;
				}
			
				fclose(ex_file1);
			
				// Check for PNG header.
				if (!memcmp(ex_file1_magic, png_magic, 4))
				{
					if (!startdat_name)
						startdat_name = ex_file_name1;
				}
				else if (!memcmp(ex_file1_magic, psp_magic, 4))  // Check for PSP header.
				{
					if (!opnssmp_name)
						opnssmp_name = ex_file_name1;
				}
				else
				{
					fprintf(stderr, "ERROR: Please check your optional files!\n");
					fclose(iso);
					fclose(pbp);
					return 0;
				}
			}
		
			// Check the second optional file.
			if (ex_file_name2)
			{
				// Read the second optional file.
				char ex_file2_magic[4] = {0x00, 0x00, 0x00, 0x00};
				FILE* ex_file2 = fopen(ex_file_name2, "rb");
				
				if (ex_file2 != NULL && fread(ex_file2_magic, 1, 4, ex_file2) != 4)
				{
					fprintf(stderr, "ERROR: Cannot read optional file 2\n");
					fclose(ex_file2);
					fclose(iso);
					fclose(pbp);
					return 0;
				}
				
				fclose(ex_file2);
				
				// Check for PNG header.
				if (!memcmp(ex_file2_magic, png_magic, 4))
				{
					if (!startdat_name)
						startdat_name = ex_file_name2;
				}
				else if (!memcmp(ex_file2_magic, psp_magic, 4))  // Check for PSP header.
				{
					if (!opnssmp_name)
						opnssmp_name = ex_file_name2;
				}
				else
				{
					fprintf(stderr, "ERROR: Please check your optional files!\n");
					fclose(iso);
					fclose(pbp);
					return 0;
				}
			}
		}
		
		// Check for custom OPNSSMP file.
		u8 *pgd_buf = NULL;
		int pgd_size = 0;
		if (opnssmp_name)
		{
			// Open file.
			FILE* opnssmp = fopen(opnssmp_name, "rb");
			
			// Check for valid file.
			if (opnssmp == NULL)
			{
				fprintf(stderr, "ERROR: Please check your OPNSSMP file!\n");
				fclose(opnssmp);
				fclose(iso);
				fclose(pbp);
				return 0;
			}

			// Get OPNSSMP file size.
			fseek(opnssmp, 0, SEEK_END);
			int opnssmp_size = ftell(opnssmp);
			fseek(opnssmp, 0, SEEK_SET);
			
			// Prepare PGD buffers.
			int pgd_block_size = 2048;
			int pgd_blocks = ((opnssmp_size + pgd_block_size - 1) &~ (pgd_block_size - 1)) / pgd_block_size;
			pgd_buf = (u8 *) malloc (0x90 + opnssmp_size + pgd_blocks * 16);
		
			// Read OPNSSMP file.
			u8 *opnssmp_buf = (u8 *) malloc (opnssmp_size);
			if ((int)fread(opnssmp_buf, 1, opnssmp_size, opnssmp) != opnssmp_size)
			{
				fprintf(stderr, "ERROR: Cannot read OPNSSMP file\n");
				fclose(opnssmp);
				fclose(iso);
				fclose(pbp);
				return 0;
			}
		
			// Encrypt OPNSSMP file with version_key.
			pgd_size = encrypt_pgd(opnssmp_buf, opnssmp_size, pgd_block_size, 1, 1, 2, version_key, pgd_buf);
			
			// Clean up.
			fclose(opnssmp);
			free(opnssmp_buf);
		}
		
		// Check for custom STARTDAT file.
		u8 *startdat_buf = NULL;
		int startdat_size = 0;
		if (startdat_name)
		{
			// Open file.
			FILE* png = fopen(startdat_name, "rb");
			
			// Check for valid file.
			if (png == NULL)
			{
				fprintf(stderr, "ERROR: Please check your STARTDAT file!\n");
				fclose(png);
				fclose(iso);
				fclose(pbp);
				return 0;
			}

			// Get STARTDAT file size.
			fseek(png, 0, SEEK_END);
			int png_size = ftell(png);
			fseek(png, 0, SEEK_SET);
			
			// Prepare STARTDAT buffer.
			startdat_size = png_size + 0x50;
			startdat_buf = (u8 *) malloc (startdat_size);
			
			// Build STARTDAT header.
			u8 sd_header_buf[0x50];
			STARTDAT_HEADER *sd_header = (STARTDAT_HEADER *)sd_header_buf;
			memset(sd_header, 0, 0x50);
			
			// Set magic STARTDAT.
			sd_header->magic[0] = 0x53;
			sd_header->magic[1] = 0x54;
			sd_header->magic[2] = 0x41;
			sd_header->magic[3] = 0x52;
			sd_header->magic[4] = 0x54;
			sd_header->magic[5] = 0x44;
			sd_header->magic[6] = 0x41;
			sd_header->magic[7] = 0x54;
			
			// Set unknown flags.
			sd_header->unk1 = 0x1;
			sd_header->unk2 = 0x1;
			
			// Set header and data size.
			sd_header->header_size = 0x50;
			sd_header->data_size = png_size;
			
			// Copy the STARTDAT header.
			memcpy(startdat_buf, sd_header, 0x50);
			
			// Read the PNG file.
			if ((int)fread(startdat_buf + 0x50, 1, png_size, png) != png_size)
			{
				fprintf(stderr, "Warning: Error reading the PNG (STARTDAT) file, ignoring it\n");
				free(startdat_buf);
				startdat_buf = NULL;
				startdat_size = 0;
			}
			
			// Clean up.
			fclose(png);
		}
		
		// Write PBP data.
		// printf("Writing PBP data...\n");
		long long table_offset = write_pbp(pbp, iso_name, content_id, np_flags, startdat_buf, startdat_size, pgd_buf, pgd_size);
		long long table_size = iso_blocks * 0x20;
		long long np_offset = table_offset - 0x100;
		int np_size = 0x100;
		
		// Write NPUMDIMG table.
		// printf("NPUMDIMG table size: %"INT64_FORMAT"d\n", table_size);
		// printf("Writing NPUMDIMG table...\n\n");
		u8 *table_buf = malloc(table_size);
		memset(table_buf, 0, table_size);
		fwrite(table_buf, table_size, 1, pbp);
		
		// Write ISO blocks.
		// printf("ISO size: %"INT64_FORMAT"d\n", iso_size);
		// printf("ISO blocks: %"INT64_FORMAT"d\n", iso_blocks);
		long long iso_offset = 0x100 + table_size;
		u8 *iso_buf = malloc(block_size * 2);
		u8 *lzrc_buf = malloc(block_size * 2);
		
		int i;
		for(i = 0; i < iso_blocks; i++)
		{
			u8 *tb = table_buf + i * 0x20;
			u8 *wbuf;
			int wsize, lzrc_size, ratio;

			// Read ISO block.
			memset(iso_buf, 0, block_size);
			if ((ftello64(iso) + block_size) > iso_size)
			{
				long long remaining = iso_size - ftello64(iso);
				if (fread(iso_buf, remaining, 1, iso) != 1)
					fprintf(stderr, "Warning: Error reading ISO block\n");
				wsize = remaining;
			}
			else
			{
				if (fread(iso_buf, block_size, 1, iso) != 1)
					fprintf(stderr, "Warning: Error reading ISO block\n");
				wsize = block_size;
			}
			
			// Set write buffer.
			wbuf = iso_buf;

			// Compress data.
			if (compress == 1)
			{
				lzrc_size = lzrc_compress(lzrc_buf, block_size * 2, iso_buf, block_size);
				memset(lzrc_buf + lzrc_size, 0, 16);
				ratio = (lzrc_size * 100) / block_size;
				
				if (ratio < RATIO_LIMIT)
				{
					wbuf = lzrc_buf;
					wsize = (lzrc_size + 15) &~ 15;
				}
			}

			// Set table entry.
			*(u32*)(tb + 0x10) = iso_offset;
			*(u32*)(tb + 0x14) = wsize;
			*(u32*)(tb + 0x18) = 0;
			*(u32*)(tb + 0x1C) = 0;

			// Encrypt block.
			sceDrmBBCipherInit(&ckey, 1, 2, header_key, version_key, (iso_offset >> 4));
			sceDrmBBCipherUpdate(&ckey, wbuf, wsize);
			sceDrmBBCipherFinal(&ckey);

			// Build MAC.
			sceDrmBBMacInit(&mkey, 3);
			sceDrmBBMacUpdate(&mkey, wbuf, wsize);
			sceDrmBBMacFinal(&mkey, tb, version_key);
			bbmac_build_final2(3, tb);

			// Encrypt table.
			encrypt_table(tb);

			// Write ISO data.
			wsize = (wsize + 15) &~ 15;
			fwrite(wbuf, wsize, 1, pbp);

			// Update offset.
			iso_offset += wsize;
			// printf("\rWriting ISO blocks: %02" INT64_FORMAT "d%%", i * 100 / iso_blocks);
		}
		// printf("\rWriting ISO blocks: 100%%\n\n");
		
		// Generate data key.
		sceDrmBBMacInit(&mkey, 3);
		sceDrmBBMacUpdate(&mkey, table_buf, table_size);
		sceDrmBBMacFinal(&mkey, data_key, version_key);
		bbmac_build_final2(3, data_key);
		
		// Forge NPUMDIMG header.
		// printf("Forging NPUMDIMG header...\n");
		NPUMDIMG_HEADER* npumdimg = forge_npumdimg((int)iso_size, (int)iso_blocks, block_basis, content_id, np_flags, version_key, header_key, data_key);
		// printf("NPUMDIMG flags: 0x%08X\n", np_flags);
		// printf("NPUMDIMG block basis: 0x%08X\n", block_basis);
		// printf("NPUMDIMG version key: 0x");
		// for (i = 0; i < 0x10; i++)
			// printf("%02X", version_key[i]);	
		// printf("\n");
		// printf("NPUMDIMG header key: 0x");
		// for (i = 0; i < 0x10; i++)
			// printf("%02X", npumdimg->header_key[i]);
		// printf("\n");
		// printf("NPUMDIMG header hash: 0x");
		// for (i = 0; i < 0x10; i++)
			// printf("%02X", npumdimg->header_hash[i]);
		// printf("\n");
		// printf("NPUMDIMG data key: 0x");
		// for (i = 0; i < 0x10; i++)
			// printf("%02X", npumdimg->data_key[i]);
		// printf("\n\n");
	
		// Update NPUMDIMG header and NP table.
		fseeko64(pbp, np_offset, SEEK_SET);
		fwrite(npumdimg, np_size, 1, pbp);
		fseeko64(pbp, table_offset, SEEK_SET);
		fwrite(table_buf, table_size, 1, pbp);
		
		// Clean up.
		fclose(iso);
		fclose(pbp);
		free(table_buf);
		free(iso_buf);
		free(lzrc_buf);
		free(npumdimg);
		
		return 0;
	}
	else
	{
		print_usage();
		return 0;
	}
}
