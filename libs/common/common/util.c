/* Linblow */
#include <common/util.h>
#include <math.h>

#define BUILD_FILE_PATH_BUFSZ (1024)

char * basename(const char *filename)
{
    char *p;
    char *p1 = strrchr(filename, '/');
    char *p2 = strrchr(filename, '\\');
    p = (uintptr_t)p1 < (uintptr_t)p2 ? p2 : p1;
    return p ? p + 1 : (char *) filename;
}

static inline int is_dir_sep(int c)
{
    return c == '/' || c == '\\';
}

/* Count redundant directory seperators (ie. 2 for "///").
   Consider a final seperator as redundant when final_sep is true. */
static u32 count_redundant_dir_sep(char const *path, int final_sep)
{
    u32 i, n;
    int is_curr, is_last;

    n = 0;
    is_last = 0;
    
    for (i = 0; path[i] != 0; ++i)
    {
        is_curr = is_dir_sep(path[i]);
        if (is_curr && is_last)
            n++;
        is_last = is_curr;
    }

    if (final_sep && i > 0 && is_curr)
        n++;
    
    return n;
}

/* Normalize src path into dst (ie. \ becomes /).
   Remove the redundant directory seperators (eg. /// becomes /).
   Remove the final directory seperator when no_final_sep is true.
   Return the dst path length (ie. position of null-terminating byte). */
static u32 normalize_file_path(char *dst, char const *src, int no_final_sep)
{
    u32 i, n;
    int is_curr, is_last;
    
    n = 0;
    is_last = 0;
    for (i = 0; src[i] != 0; is_last = is_curr, ++i)
    {
        //TODO detect and remove "./" sequences
        is_curr = is_dir_sep(src[i]);
        // Ignore redundant seperator.
        if (is_last && is_curr)
            continue;
        // Normalize the previous seperator.
        if (is_last && !is_curr)
            dst[n-1] = '/';
        dst[n++] = src[i];
    }

    // Normalize or remove final separator
    if (n > 0 && is_curr)
    {
        if (no_final_sep)
            dst[--n] = '\0';
        else
            dst[n-1] = '/';
    }

    dst[n] = '\0';
    return n;
}

/**
 * Append file path b to a into out.
 * Insert a directory separator between a and b only when necessary.
 * Normalize the resulting path (no redundant and final seperators).
 * 
 * The resulting path is written to the output buf only if it has enough 
 * space to store the whole string, including the null-terminating byte.
 * 
 * IN:  a: foo\bar/
 *      b: //hello/\/bar.ext
 * OUT: foo/bar/hello/bar.ext
 * 
 * @param[out] out   Output buf to store the resulting path
 * @param[in]  outsz Buf capacity
 * @param[out] pLen  Optional pointer to receive the resulting path length
 * @param[in]  a     File path 1
 * @param[in]  b     File path 2 (printf-format)
 * @param[in]  ...   Args for file path 2 format string
 * 
 * @return   0 success; 
 *          -1 error formatting b, or internal buf is too small to format it; 
 *         > 0 number of additional bytes needed to store the string.
 */
int build_file_path(char *out, size_t outsz, size_t *pLen, const char *a, const char *b, ...)
{
    va_list va;
    char _b[BUILD_FILE_PATH_BUFSZ];
    int _b_len;
    char *outp = out;
    size_t a_len, b_len, a_len_rm, b_len_rm;
    size_t len, total_size;
    int need_sep;

    _b_len = 0;
    if (b)
    {
        va_start(va, b);
        _b_len = vsnprintf(_b, sizeof(_b), b, va);
        va_end(va);
        if (_b_len < 0 || _b_len >= sizeof(_b))
            return -1;
        b = (const char *)_b;
    }

    if (!a) a = "";
    if (!b) b = "";
    
    a_len = strlen(a);
    b_len = _b_len ? _b_len : strlen(b);
    a_len_rm = count_redundant_dir_sep(a, 1);
    b_len_rm = count_redundant_dir_sep(b, 1);
    need_sep = b_len > 0 && !is_dir_sep(b[0]);

    if (a_len == a_len_rm && b_len == b_len_rm)
    {
        /* Both a and b only contain slashes. */
        need_sep = 1;
        total_size = 2;
    }
    else
    {
        total_size  = a_len - a_len_rm;
        total_size += b_len - b_len_rm;
        total_size += need_sep + 1;
    }
#if 0
    printf("-\n");
    printf("a_len      = %u\n", a_len);
    printf("a_len_rm   = %u\n", a_len_rm);
    printf("b_len      = %u\n", b_len);
    printf("b_len_rm   = %u\n", b_len_rm);
    printf("total_size = %u\n", total_size);
    printf("-\n");
#endif
    if (outsz < total_size)
        return total_size - outsz;

    len = normalize_file_path(outp, a, 1);
    outp += len;
    if (need_sep)
    {
        len = normalize_file_path(outp, "/", 0);
        outp += len;
    }
    len = normalize_file_path(outp, b, 1);
    outp += len;

    if (pLen)
        *pLen = outp - out;
    
    return 0;
}

int getfilesize(const char *path, int *pSize)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return 1;
    if (pSize)
        *pSize = (int)st.st_size;
    return 0;
}

int format_file_size(char * buf, size_t bufsz, int nbytes, int si, int dp)
{
    /* JS source https://stackoverflow.com/a/14919494 */
    static const char * units_si[] = {"kB","MB","GB","TB","PB","EB","ZB","YB"};
    static const char * units[] = {"KiB","MiB","GiB","TiB","PiB","EiB","ZiB","YiB"};
    static const int n_units = sizeof(units) / sizeof(char *);
    int thresh = si ? 1000 : 1024;
    int u = -1;
    double r, n;

    if (abs(nbytes) < thresh)
        return snprintf(buf, bufsz, "%d B", nbytes);

    r = pow(10, (double)dp);
    n = (double)nbytes;
    do {
        n /= thresh;
        ++u;
    }
    while (round(fabs(n) * r) / r >= thresh && u < n_units - 1);
    
    return snprintf(buf, bufsz, "%.*f %s", dp, n, si ? units_si[u] : units[u]);
}

