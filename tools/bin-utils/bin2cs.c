// SPDX-FileCopyrightText: PSPDEV Open Source Project
// SPDX-FileContributor: Sjeep
// SPDX-FileContributor: Linblow
// SPDX-License-Identifier: BSD-3-Clause
// PSP Software Development Kit - https://github.com/pspdev
/* Original by Sjeep, refactored and modified by Linblow. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_ALIGNMENT (16)

#define RETERR(_ret, fmt, ...)                \
    do {                                      \
        ret = _ret;                           \
        fprintf(stderr, fmt, ## __VA_ARGS__); \
        goto end;                             \
    }                                         \
    while (0)


#if BIN2C

static int write_c_array_header(FILE *out, const char *source, const char *label, int alignment, const char *data, size_t len)
{
    size_t i;

    fprintf(out, "// Binary file source: %s\n", source);
    fprintf(out, "#ifndef __%s__\n", label);
    fprintf(out, "#define __%s__\n\n", label);
    fprintf(out, "static unsigned int size_%s = %lu;\n", label, len);
    fprintf(out, "static unsigned int __attribute__((alias(\"size_%s\"))) %s_size;\n", label, label);
    fprintf(out, "static unsigned char %s[] __attribute__((aligned(%d))) = {", label, alignment);

    for (i = 0; i < len; ++i) {
        if ((i % 16) == 0)
            fprintf(out, "\n\t");
        
        fprintf(out, "0x%02x, ", (unsigned char)data[i]);
    }

    fprintf(out, "\n};\n\n#endif\n");

    return ferror(out) != 0;
}

#elif BIN2S

static int write_assembly_source(FILE *out, const char *source, const char *label, int alignment, const char *data, size_t len)
{
    size_t i;

    fprintf(out, "# Binary file source: %s\n", source);
    fprintf(out, ".sdata\n\n"
                 ".globl size_%s\n"
                 ".globl %s_size\n"
                 "size_%s:\n"
                 "%s_size:\n"
                 "\t.word %lu\n\n", 
                 label, label, label, label, len);
    fprintf(out, ".data\n\n");
    fprintf(out, ".balign %d\n\n", alignment);
    fprintf(out, ".globl %s\n", label);
    fprintf(out, "%s:\n\n", label);

    for (i = 0; i < len; ++i) {
        if ((i % 16) == 0)
            fprintf(out, "\n\t.byte 0x%02x", (unsigned char)data[i]);
        else
            fprintf(out, ", 0x%02x", (unsigned char)data[i]);
    }

    fprintf(out, "\n");

    return ferror(out) != 0;
}

#endif

static char * basename(const char *filename)
{
    char *p;
    char *p1 = strrchr(filename, '/');
    char *p2 = strrchr(filename, '\\');
    p = (uintptr_t)p1 < (uintptr_t)p2 ? p2 : p1;
    return p ? p + 1 : (char *) filename;
}

static void show_help()
{
#if BIN2C
    printf("bin2c - by Sjeep, Linblow\n"
           "Convert a binary file to a C array source file.\n"
           "Usage: bin2c <infile> <outfile> <label> [<alignment>]\n");
#elif BIN2S
    printf("bin2s - by Sjeep, Linblow\n"
           "Convert a binary file to an assembly source file.\n"
           "Usage: bin2s <infile> <outfile> <label> [<alignment>]\n");
#endif
}

int main(int argc, char *argv[])
{
    int ret;
    char *buf;
    FILE *in, *out;
    char *in_path, *out_path, *label;
    int pos, alignment;
    size_t len;

    buf = NULL;
    in = NULL;
    out = NULL;

    if (argc < 4) {
        show_help();
        return 1;
    }

    in_path = argv[1];
    out_path = argv[2];
    label = argv[3];
    
    alignment = argc >= 5 ? atoi(argv[4]) : DEFAULT_ALIGNMENT;
    if (alignment == 0)
        fprintf(stderr, "Warning alignment is zero.\n");
    if ((alignment - 1) & alignment)
        RETERR(1, "Error alignment must be a power of 2.\n");

    in = fopen(in_path, "rb");
    if (in == NULL) 
        RETERR(1, "Error opening %s for reading.\n", in_path);

    fseek(in, 0, SEEK_END);
    pos = ftell(in);
    if (pos < 0)
        RETERR(1, "Error getting %s file size.\n", in_path);
    len = (size_t)pos;
    fseek(in, 0, SEEK_SET);

    buf = malloc(len);
    if (buf == NULL)
        RETERR(1, "Failed to allocate memory.\n");
    
    if (fread(buf, 1, len, in) != len)
        RETERR(1, "Failed to read file.\n");
    
    fclose(in);
    in = NULL;

    out = fopen(out_path, "w+");
    if (out == NULL) 
        RETERR(1, "Failed to open/create %s.\n", out_path);

#if BIN2C
    if (write_c_array_header(out, basename(in_path), label, alignment, buf, len) != 0)
        RETERR(1, "Error writing output file %s.\n", out_path);
#elif BIN2S
    if (write_assembly_source(out, basename(in_path), label, alignment, buf, len) != 0)
        RETERR(1, "Error writing output file %s.\n", out_path);
#endif

    ret = 0;
end:
    if (out != NULL)
        fclose(out);
    if (buf != NULL)
        free(buf);
    if (in != NULL)
        fclose(in);

    return ret;
}
