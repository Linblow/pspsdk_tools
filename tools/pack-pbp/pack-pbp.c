// SPDX-FileCopyrightText: 2022 Linblow <dev@linblow.com>
// SPDX-License-Identifier: BSD-3-Clause
#include <common/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* getopt is GNU Lesser General Public version 2.1, or (at your option) any later version. 
   See https://choosealicense.com/licenses/lgpl-2.1/ */
#include <getopt.h>

#include "pbp.h"

#define DEFAULT_HUMAN_SIZE       (1)
#define DEFAULT_SIZE_PRECISION   (2)
#define DEFAULT_PBP_VERSION      (1) // 0: v1.0; 1: v1.1
#define DEFAULT_OUTPUT_PATH      "EBOOT.PBP"

#define READBUFSZ (16 * 1024 * 1024) // 16 MiB

enum {
   ERR_OK = 0,
   ERR_FAIL = 1, // Generic failure
   ERR_IO_OPEN,
   ERR_IO_CLOSE,
   ERR_IO_READ,
   ERR_IO_WRITE,
   ERR_IO_GETFILESIZE,
   ERR_IO_STAT,
   ERR_MALLOC
};

typedef union {
   struct {
      char *param;
      char *icon0;
      char *icon1;
      char *pic0;
      char *pic1;
      char *snd0;
      char *data_psp;
      char *data_psar;
   };
   char *path[8];
} pbp_files_path;

static int g_quiet = 0;
static int g_verbose = 0;

#define ENDERR(retv, msg)              \
do {                                   \
   ret = (retv);                       \
   printferr(msg);                     \
   goto end;                           \
} while (0)

#define ENDERRF(retv, fmt, ...)         \
do {                                   \
   ret = (retv);                       \
   printferr(fmt, ## __VA_ARGS__ );    \
   goto end;                           \
} while (0)

static int printferr(const char *fmt, ...)
{
   int n;
   char buf[512];
   va_list va;
   n = 0;
   if (!g_quiet)
   {
      va_start(va, fmt);
      n = vsnprintf(buf, sizeof(buf), fmt, va);
      va_end(va);
      if (n >= 0 && n < (int)sizeof(buf))
         fputs(buf, stderr);
   }
   return n;
}

static const char * get_pbpentry_name(int index)
{
   static const char *ent[] = 
   {
      "PARAM.SFO",
      "ICON0.PNG",
      "ICON1.PNG/PMF",
      "PIC0.PNG",
      "PIC1(T).PNG",
      "SND0.AT3",
      "DATA.PSP",
      "DATA.PSAR"
   };
   return (index >= 0 && index < 8) ? ent[index] : "";
}

static int write_pbp(const char *path, 
                     const pbp_files_path *files, 
                     int pbp_version, 
                     int human_size, 
                     int use_si_units, 
                     int size_precision)
{
   int ret;
   int i;
   int filesize[8];
   char strsize[32];
   size_t pbpsize, readsize;
   FILE *fout = NULL;
   FILE *fp = NULL;
   char *buf = NULL;
   ScePBPHeader hdr;

   fout = fopen(path, "wb");
   if (fout == NULL)
      ENDERRF(ERR_IO_OPEN, "Cannot open the output file (%s)\n", path);

   buf = malloc(READBUFSZ);
   if (buf == NULL)
      ENDERR(ERR_MALLOC, "Cannot allocate the file read buffer\n");

   /* Init PBP header */
   memset(&hdr, 0, sizeof(ScePBPHeader));
   memcpy(hdr.magic, PBP_MAGIC_STR, 4);
   SW(&hdr.version, pbp_version ? 0x00010001 : 0x00010000); // 1.1 or 1.0

   /* Get file size */
   pbpsize = sizeof(ScePBPHeader);
   for (i = 0; i < 8; ++i)
   {
      filesize[i] = 0;
      if (files->path[i] && getfilesize(files->path[i], &filesize[i]) != 0)
         ENDERRF(ERR_IO_GETFILESIZE, "Cannot get the file size (%s)\n", files->path[i]);
      if (i == 0)
         SW(&hdr.file_offset[i], sizeof(ScePBPHeader));
      else
         SW(&hdr.file_offset[i], hdr.file_offset[i-1] + filesize[i-1]);
      pbpsize += filesize[i];
   }

   if (1 != fwrite(&hdr, sizeof(ScePBPHeader), 1, fout))
      ENDERRF(ERR_IO_WRITE, "Cannot write out the file header (%s)\n", path);

   /* Append file data. */
   for (i = 0; i < 8; ++i)
   {
      if (g_verbose)
      {
         buf[0] = 0;
         if (files->path[i])
            sprintf(buf, " (%s)", basename(files->path[i]));
         if (human_size)
            format_file_size(strsize, sizeof(strsize), filesize[i], use_si_units, size_precision);
         else
            sprintf(strsize, "%d B", filesize[i]);
         printf("[%d] %-13s %13s%s\n", i+1, get_pbpentry_name(i), strsize, buf);
      }
      if (files->path[i] == NULL)
         continue;
      
      fp = fopen(files->path[i], "rb");
      if (fp == NULL)
         ENDERRF(ERR_IO_OPEN, "Cannot open the file (%s)\n", files->path[i]);
      while (filesize[i] > 0)
      {
         readsize = filesize[i] >= READBUFSZ ? READBUFSZ : filesize[i];
         if (readsize != fread(buf, 1, readsize, fp))
            ENDERRF(ERR_IO_READ, "Cannot read in the file data (%s)\n", files->path[i]);
         if (readsize != fwrite(buf, 1, readsize, fout))
            ENDERRF(ERR_IO_WRITE, "Cannot write out the file data (%s)\n", path);
         filesize[i] -= readsize;
      }
      fclose(fp);
      fp = NULL;
   }

   if (0 != fclose(fout))
      ENDERRF(ERR_IO_CLOSE, "Cannot close the output file handle (%s)\n", path);
   
   if (g_verbose)
   {
      if (human_size)
         format_file_size(strsize, sizeof(strsize), pbpsize, use_si_units, size_precision);
      else
         sprintf(strsize, "%d bytes", pbpsize);
      printf("Created %s of %s\n", basename(path), strsize);
   }

   fout = NULL;
   ret = ERR_OK;
   
end:
   if (fp != NULL)
      fclose(fp);
   if (buf != NULL)
      free(buf);
   if (fout != NULL)
      fclose(fout);
   
   return ret;
}

static void show_help(char *argv[])
{
   const char *prog = basename(argv[0]);
   printf(
      "Create a Sony PSP file archive, v%s by Linblow.\n"
      "Usage:\n"
      "  %s [options] [-o <output_pbp>] -1 <sfo>\n"
      "  %s [options] <output_pbp> <sfo> [<icon0> <icon1> <pic0> <pic1> <snd0> <data> <psar>]\n\n"
      "Options:\n"
      "  -h --help               Show this screen\n"
      "  -v --verbose[=<level>]  Print result to stdout (unless -q was passed)\n"
      "                          Level 0 for disable, 1 for verbose [default: 1]\n"
      "  -q --quiet              Disable all output to stdout/stderr\n"
      "  -H --human-size[=<precision]\n"
      "                          Print sizes in human readable format (e.g., 1K 234M 2G)\n"
      "                          Pass off/no to disable [default: enabled with precision=2]\n"
      "     --si                 Use SI units (i.e. power of 10 instead of 2)\n\n"
      "  -o --out=<path>         Set output PBP file path [default: EBOOT.PBP]\n"
      "  -x --ver=<ver>          Set PBP header version: 1.0 (or 0), 1.1 (or 1) [default: 1.1]\n\n"
      "  -1 --param=<path>       Set PARAM.SFO file path\n"
      "  -2 --icon0=<path>       Set ICON0.PNG file path\n"
      "  -3 --icon1=<path>       Set ICON1.PNG / ICON1.PMF file path\n"
      "  -4 --pic0=<path>        Set PIC0.PNG file path\n"
      "  -5 --pic1=<path>        Set PIC1.PNG / PICT1.PNG file path\n"
      "  -6 --snd0=<path>        Set SND0.AT3 file path\n"
      "  -7 --data=<path>        Set DATA.PSP file path\n"
      "  -8 --psar=<path>        Set DATA.PSAR file path\n"
      , TOOL_VERSION, prog, prog
   );
}

typedef struct {
   int help;
   u32 verbose;
   int quiet;
   int human_size;
   int use_si_units;
   int size_precision; // -1 for default
   int pbp_version; // 0 for 1.0, 1 for 1.1
   char *output_path;
   pbp_files_path files;
} parsed_args_t;

static int is_skip_argval(const char *val)
{
   return stricmp(val, "NULL") == 0 || strcmp(val, "-") == 0;
}

static int is_false_bool_argval(const char *val)
{
   return stricmp(val, "n") == 0      || 
          stricmp(val, "no") == 0     || 
          stricmp(val, "off") == 0    || 
          stricmp(val, "false") == 0;
}

static int parse_args(int argc, char *argv[], parsed_args_t *args)
{
   //TODO add s/scan option to scan for the files in the specified directory
   //TODO add i/info option to print info about a specified package
   struct option opts[] =
   {
      {"help",       no_argument,       0, 'h'},
      {"verbose",    optional_argument, 0, 'v'},
      {"quiet",      no_argument,       0, 'q'},
      {"human-size", optional_argument, 0, 'H'},
      {"si",         no_argument, &args->use_si_units, 1},
      {"out",        required_argument, 0, 'o'},
      {"ver",        required_argument, 0, 'x'}, // PBP version in header
      {"param",      required_argument, 0, '1'},
      {"icon0",      required_argument, 0, '2'},
      {"icon1",      required_argument, 0, '3'},
      {"pic0",       required_argument, 0, '4'},
      {"pic1",       required_argument, 0, '5'},
      {"snd0",       required_argument, 0, '6'},
      {"data",       required_argument, 0, '7'},
      {"psar",       required_argument, 0, '8'},
      {0, 0, 0, 0}
   };
   int optindex = 0;
   int c;
   while (1)
   {
      c = getopt_long(argc, argv, "hH::o:pqv::x:1:2:3:4:5:6:7:8:", opts, &optindex);
      if (c == -1)
         break;
      switch (c)
      {
         case 'h':
            args->help = 1;
            break;
         case 'H':
            args->human_size = optarg && is_false_bool_argval(optarg) ? 0 : 1;
            args->size_precision = optarg ? strtol(optarg, NULL, 10) : -1;
            break;
         case 'v':
            args->verbose = optarg ? strtoul(optarg, NULL, 10) : 1;
            break;
         case 'q':
            args->quiet = 1;
            break;
         case 'o':
            args->output_path = optarg;
            break;
         case 'x':
            if (strcmp(optarg, "1.0") == 0 || strcmp(optarg, "0") == 0)
               args->pbp_version = 0;
            else if (strcmp(optarg, "1.1") == 0 || strcmp(optarg, "1") == 0)
               args->pbp_version = 1;
            break;
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
               args->files.path[c - '1'] = optarg;
            break;
         case '?':
            /* getopt_long already printed an error message. */
            return 1;
            break;
      }
   }
   return 0;
}

int main(int argc, char *argv[])
{
   int ret;
   int i;
   parsed_args_t args;
   memset(&args, 0, sizeof(args));
   args.verbose = 1;
   args.human_size = DEFAULT_HUMAN_SIZE;
   args.use_si_units = 0;
   args.size_precision = -1; // will use the default
   args.pbp_version = DEFAULT_PBP_VERSION;
   args.output_path = DEFAULT_OUTPUT_PATH;
   
   if (0 != parse_args(argc, argv, &args) || args.help)
   {
      show_help(argv);
      return 0;
   }

   /* These must be set after the call to parse_args . */
   g_quiet = args.quiet;
   g_verbose = !g_quiet && args.verbose;

   /* Set the default precision for formatted sizes. */
   if (args.size_precision < 0)
      args.size_precision = DEFAULT_SIZE_PRECISION;

   /* Legacy pack-pbp arguments. */
   // <output_pbp> <sfo> [<icon0> <icon1> <pic0> <pic1> <snd0> <data> <psar>]
   for (i = 0; optind < argc && i < 9; ++i, ++optind)
   {
      if (i == 0)
         args.output_path = is_skip_argval(argv[optind]) ? DEFAULT_OUTPUT_PATH : argv[optind];
      else if ( ! is_skip_argval(argv[optind]))
         args.files.path[i - 1] = argv[optind];
   }

   /* Warn about empty path. */ 
   for (i = 0; i < 8; ++i)
   {
      if (args.files.path[i] && strlen(args.files.path[i]) == 0)
      {
         printferr("WARNING: the specified %s file path is empty, thus ignored\n", get_pbpentry_name(i));
         args.files.path[i] = NULL;
      }
   }

   if (args.output_path && strlen(args.output_path) == 0)
      ENDERR(ERR_FAIL, "The specified output path is empty\n");
   if (args.files.param == NULL)
      ENDERR(ERR_FAIL, "The PARAM.SFO file path is required\n");

   ret = write_pbp(args.output_path, &args.files, args.pbp_version, args.human_size, args.use_si_units, args.size_precision);
   if (ret != 0)
      remove(args.output_path);
end:
   return ret;
}
