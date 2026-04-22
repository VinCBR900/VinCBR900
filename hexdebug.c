/*
 * hexdebug.c - DOS DEBUG-style hex viewer/editor
 * Version: 1.2.0 Copyright Vincent Crabtree 2026
 * MIT License see LICENCE
 *
 * Interactive command-line hex viewer/editor for binary files.
 * Displays 16 rows of 16 bytes with ASCII preview and supports
 * paging, editing, searching, appending, and saving.
 *
 * Version history:
 *   1.2.0 - Add backup-on-write behavior, --skip-backups option, and d [addr] [count].
 *   1.1.1 - Search repeat and wrap behavior refinements.
 */

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define strcasecmp _stricmp
#endif

#define ROW_BYTES 16
#define PAGE_ROWS 16
#define PAGE_BYTES (ROW_BYTES * PAGE_ROWS)
#define HEXDEBUG_VERSION "1.2.0"

static const char *signon = "\nhexdebug v" HEXDEBUG_VERSION " - Interactive DOS DEBUG-style hex file viewer/editor\n";

static volatile sig_atomic_t g_interrupted = 0;

static void on_sigint(int sig) {
    (void)sig;
    g_interrupted = 1;
}

typedef struct {
    uint8_t *data;
    size_t size;
    size_t cap;
    bool modified;
    char filename[1024];
    size_t view_addr;
    size_t search_addr;
    bool   has_search;
    uint8_t search_pat[4096];
    size_t  search_pat_len;
    bool    search_pat_is_text;
} HexFile;

static void print_help(void) {
    puts(signon);
    puts("Usage:");
    puts("  hexdebug [--skip-backups] <file>");
    puts("");
    puts("Commands:");
    puts("  <enter>               Display next 16 rows");
    puts("  p                     Display previous 16 rows");
    puts("  d [addr] [count]      Display bytes from addr (default 0, count defaults to 256)");
    puts("  e addr aa bb ...      Edit bytes at addr using hex values");
    puts("  e addr \"text\"       Edit bytes at addr using ASCII text");
    puts("  a file                Append another file");
    puts("  w                     Write current buffer to file (creates .bak unless --skip-backups)");
    puts("  s aa bb ...           Search for byte sequence");
    puts("  s \"text\"            Search for ASCII text");
    puts("  h or ?                Show help");
    puts("  n file                Change current filename");
    puts("  r                     Report current filename and file size");
    puts("  q or ESC              Quit (prompts to save if modified)");
    puts("  Ctrl-C                Quit immediately");
}

static void print_row(const uint8_t *data, size_t size, size_t addr) {
    printf("%08zx  ", addr);
    for (size_t i = 0; i < ROW_BYTES; i++) {
        if (addr + i < size) {
            printf("%02X ", data[addr + i]);
        } else {
            printf("   ");
        }
    }
    printf(" ");
    for (size_t i = 0; i < ROW_BYTES; i++) {
        if (addr + i < size) {
            unsigned char c = data[addr + i];
            putchar(isprint(c) ? (int)c : '.');
        } else {
            putchar(' ');
        }
    }
    putchar('\n');
}

static void display_page(HexFile *hf, size_t start) {
    if (start > hf->size) {
        start = hf->size;
    }
    hf->view_addr = start;
    size_t addr = start;
    for (int row = 0; row < PAGE_ROWS && addr < hf->size; row++) {
        print_row(hf->data, hf->size, addr);
        addr += ROW_BYTES;
    }
    hf->view_addr = addr;
}

static void display_count(HexFile *hf, size_t start, size_t count) {
    if (start > hf->size) {
        start = hf->size;
    }
    if (count == 0) {
        hf->view_addr = start;
        return;
    }

    size_t end = start + count;
    if (end < start || end > hf->size) {
        end = hf->size;
    }

    size_t addr = start;
    while (addr < end) {
        print_row(hf->data, hf->size, addr);
        if (end - addr <= ROW_BYTES) {
            addr = end;
            break;
        }
        addr += ROW_BYTES;
    }
    hf->view_addr = addr;
}

static bool ensure_capacity(HexFile *hf, size_t needed) {
    if (needed <= hf->cap) {
        return true;
    }
    size_t new_cap = hf->cap ? hf->cap : 1024;
    while (new_cap < needed) {
        if (new_cap > SIZE_MAX / 2) {
            new_cap = needed;
            break;
        }
        new_cap *= 2;
    }
    uint8_t *p = (uint8_t *)realloc(hf->data, new_cap);
    if (!p) {
        fprintf(stderr, "error: out of memory\n");
        return false;
    }
    hf->data = p;
    hf->cap = new_cap;
    return true;
}

static bool load_file(HexFile *hf, const char *name) {
    FILE *fp = fopen(name, "rb");
    if (!fp) {
        fprintf(stderr, "error: cannot open %s: %s\n", name, strerror(errno));
        return false;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        fprintf(stderr, "error: cannot seek %s\n", name);
        return false;
    }
    long len = ftell(fp);
    if (len < 0) {
        fclose(fp);
        fprintf(stderr, "error: cannot get size of %s\n", name);
        return false;
    }
    rewind(fp);
    if (!ensure_capacity(hf, (size_t)len)) {
        fclose(fp);
        return false;
    }
    size_t nread = fread(hf->data, 1, (size_t)len, fp);
    fclose(fp);
    if (nread != (size_t)len) {
        fprintf(stderr, "error: failed reading %s\n", name);
        return false;
    }
    hf->size = (size_t)len;
    hf->modified = false;
    hf->view_addr = 0;
    strncpy(hf->filename, name, sizeof(hf->filename) - 1);
    hf->filename[sizeof(hf->filename) - 1] = '\0';
    return true;
}

static bool file_exists(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;
    fclose(fp);
    return true;
}

static bool copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) {
        fprintf(stderr, "error: cannot open %s for backup: %s\n", src, strerror(errno));
        return false;
    }
    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        fprintf(stderr, "error: cannot create backup %s: %s\n", dst, strerror(errno));
        return false;
    }

    unsigned char buf[8192];
    size_t nread = 0;
    bool ok = true;
    while ((nread = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, nread, out) != nread) {
            ok = false;
            break;
        }
    }
    if (ferror(in)) ok = false;
    if (fclose(out) != 0) ok = false;
    fclose(in);

    if (!ok) {
        fprintf(stderr, "error: backup copy failed (%s -> %s)\n", src, dst);
    }
    return ok;
}

static bool create_backup_if_needed(const char *path, bool skip_backups) {
    if (skip_backups || !file_exists(path)) {
        return true;
    }

    char backup_path[1152];
    if (snprintf(backup_path, sizeof(backup_path), "%s.bak", path) >= (int)sizeof(backup_path)) {
        fprintf(stderr, "error: backup path too long\n");
        return false;
    }

    if (file_exists(backup_path)) {
        if (skip_backups) {
            return true;
        }
        printf("backup already exists: %s\n", backup_path);
        printf("overwrite backup? (y/n): ");
        fflush(stdout);
        char ans[32];
        if (!fgets(ans, sizeof(ans), stdin)) {
            return false;
        }
        if (ans[0] != 'y' && ans[0] != 'Y') {
            puts("write canceled");
            return false;
        }
    }

    return copy_file(path, backup_path);
}

static bool save_file(HexFile *hf, bool skip_backups) {
    if (!create_backup_if_needed(hf->filename, skip_backups)) {
        return false;
    }

    FILE *fp = fopen(hf->filename, "wb");
    if (!fp) {
        fprintf(stderr, "error: cannot write %s: %s\n", hf->filename, strerror(errno));
        return false;
    }
    size_t nw = fwrite(hf->data, 1, hf->size, fp);
    fclose(fp);
    if (nw != hf->size) {
        fprintf(stderr, "error: write failed for %s\n", hf->filename);
        return false;
    }
    hf->modified = false;
    printf("saved %zu bytes to %s\n", hf->size, hf->filename);
    return true;
}

static bool parse_hex_u8(const char *tok, uint8_t *out) {
    if (!tok || !*tok) return false;
    char *end = NULL;
    unsigned long v = strtoul(tok, &end, 16);
    if (*end != '\0' || v > 0xFFUL) return false;
    *out = (uint8_t)v;
    return true;
}

static bool parse_hex_addr(const char *tok, size_t *out) {
    if (!tok || !*tok) return false;
    char *end = NULL;
    unsigned long long v = strtoul(tok, &end, 16);
    if (*end != '\0') return false;
    *out = (size_t)v;
    return true;
}

static char *skip_ws(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

static void do_edit(HexFile *hf, char *args) {
    args = skip_ws(args);
    if (*args == '\0') {
        fprintf(stderr, "error: e requires address\n");
        return;
    }

    char *addr_end = args;
    while (*addr_end && !isspace((unsigned char)*addr_end)) addr_end++;
    char saved = *addr_end;
    *addr_end = '\0';

    size_t addr;
    bool ok_addr = parse_hex_addr(args, &addr);
    *addr_end = saved;
    if (!ok_addr) {
        fprintf(stderr, "error: invalid address\n");
        return;
    }

    char *rest = skip_ws(addr_end);
    if (*rest == '\0') {
        fprintf(stderr, "error: e requires bytes or quoted text\n");
        return;
    }

    if (*rest == '"') {
        rest++;
        char *endq = strrchr(rest, '"');
        if (!endq) {
            fprintf(stderr, "error: missing closing quote\n");
            return;
        }
        *endq = '\0';
        size_t len = strlen(rest);
        if (!ensure_capacity(hf, addr + len)) return;
        memcpy(hf->data + addr, rest, len);
        if (addr + len > hf->size) hf->size = addr + len;
        hf->modified = true;
        return;
    }

    size_t pos = addr;
    char *cursor = rest;
    while (*cursor) {
        cursor = skip_ws(cursor);
        if (*cursor == '\0') break;
        char *tok_end = cursor;
        while (*tok_end && !isspace((unsigned char)*tok_end)) tok_end++;
        char sv = *tok_end;
        *tok_end = '\0';

        uint8_t b;
        bool ok = parse_hex_u8(cursor, &b);
        *tok_end = sv;
        if (!ok) {
            fprintf(stderr, "error: invalid byte value '%s'\n", cursor);
            return;
        }
        if (!ensure_capacity(hf, pos + 1)) return;
        hf->data[pos++] = b;
        cursor = tok_end;
    }

    if (pos == addr) {
        fprintf(stderr, "error: e requires bytes or quoted text\n");
        return;
    }
    if (pos > hf->size) hf->size = pos;
    hf->modified = true;
}

static void do_append(HexFile *hf, char *args) {
    args = skip_ws(args);
    if (*args == '\0') {
        fprintf(stderr, "error: a requires file\n");
        return;
    }
    FILE *fp = fopen(args, "rb");
    if (!fp) {
        fprintf(stderr, "error: cannot open %s: %s\n", args, strerror(errno));
        return;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        fprintf(stderr, "error: cannot seek %s\n", args);
        return;
    }
    long len = ftell(fp);
    if (len < 0) {
        fclose(fp);
        fprintf(stderr, "error: cannot size %s\n", args);
        return;
    }
    rewind(fp);
    size_t old_size = hf->size;
    if (!ensure_capacity(hf, old_size + (size_t)len)) {
        fclose(fp);
        return;
    }
    size_t nread = fread(hf->data + old_size, 1, (size_t)len, fp);
    fclose(fp);
    if (nread != (size_t)len) {
        fprintf(stderr, "error: failed reading %s\n", args);
        return;
    }
    hf->size = old_size + (size_t)len;
    hf->modified = true;
}

static void format_pattern(const uint8_t *pat, size_t len, bool is_text, char *buf, size_t bufsz) {
    size_t pos = 0;
    if (is_text) {
        pos += (size_t)snprintf(buf + pos, bufsz - pos, "\"");
        for (size_t i = 0; i < len && pos + 2 < bufsz; i++)
            pos += (size_t)snprintf(buf + pos, bufsz - pos, "%c", (char)pat[i]);
        snprintf(buf + pos, bufsz - pos, "\"");
    } else {
        for (size_t i = 0; i < len && pos + 4 < bufsz; i++)
            pos += (size_t)snprintf(buf + pos, bufsz - pos, i ? " %02X" : "%02X", pat[i]);
    }
}

static void do_search(HexFile *hf, char *args) {
    args = skip_ws(args);

    uint8_t  tmp[4096];
    uint8_t *pat     = NULL;
    size_t   pat_len = 0;
    bool     new_pat = (*args != '\0');   /* empty args → repeat last search */

    if (new_pat) {
        if (*args == '"') {
            args++;
            char *endq = strrchr(args, '"');
            if (!endq) {
                fprintf(stderr, "error: missing closing quote\n");
                return;
            }
            *endq  = '\0';
            pat     = (uint8_t *)args;
            pat_len = strlen(args);
            hf->search_pat_is_text = true;
        } else {
            hf->search_pat_is_text = false;
            char *tok = strtok(args, " \t\r\n");
            while (tok) {
                if (pat_len >= sizeof(tmp)) {
                    fprintf(stderr, "error: search pattern too long\n");
                    return;
                }
                if (!parse_hex_u8(tok, &tmp[pat_len])) {
                    fprintf(stderr, "error: invalid byte value '%s'\n", tok);
                    return;
                }
                pat_len++;
                tok = strtok(NULL, " \t\r\n");
            }
            if (pat_len == 0) {
                fprintf(stderr, "error: empty search pattern\n");
                return;
            }
            pat = tmp;
        }

        /* Store pattern in hf for future repeats */
        if (pat_len > sizeof(hf->search_pat)) {
            fprintf(stderr, "error: search pattern too long\n");
            return;
        }
        memcpy(hf->search_pat, pat, pat_len);
        hf->search_pat_len = pat_len;
        hf->search_addr    = 0;      /* start from beginning for a new pattern */
        hf->has_search     = true;
    } else {
        /* Repeat: advance one byte past last hit so we don't re-find it */
        if (!hf->has_search || hf->search_pat_len == 0) {
            fprintf(stderr, "error: no previous search pattern\n");
            return;
        }
        pat     = hf->search_pat;
        pat_len = hf->search_pat_len;
    }

    char patbuf[256];
    format_pattern(pat, pat_len, hf->search_pat_is_text, patbuf, sizeof(patbuf));

    for (size_t i = hf->search_addr; i + pat_len <= hf->size; i++) {
        if (memcmp(hf->data + i, pat, pat_len) == 0) {
            printf("found %s at %08zx\n", patbuf, i);
            display_page(hf, i);
            hf->search_addr = i + 1;
            return;
        }
    }

    if (hf->search_addr > 0) {
        puts("not found from here — wrapping to start");
        for (size_t i = 0; i + pat_len <= hf->search_addr && i + pat_len <= hf->size; i++) {
            if (memcmp(hf->data + i, pat, pat_len) == 0) {
                printf("found %s at %08zx\n", patbuf, i);
                display_page(hf, i);
                hf->search_addr = i + 1;
                return;
            }
        }
    }

    puts("not found");
    hf->search_addr = 0;
}

static bool confirm_save_if_needed(HexFile *hf, bool skip_backups) {
    if (!hf->modified) return true;
    printf("file modified. save changes? (y/n): ");
    fflush(stdout);
    char ans[32];
    if (!fgets(ans, sizeof(ans), stdin)) return false;
    if (ans[0] == 'y' || ans[0] == 'Y') {
        return save_file(hf, skip_backups);
    }
    return true;
}

int main(int argc, char **argv) {
    signal(SIGINT, on_sigint);

    bool skip_backups = false;
    const char *input_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            print_help();
            return 1;
        } else if (!strcmp(argv[i], "--skip-backups")) {
            skip_backups = true;
        } else if (input_file == NULL) {
            input_file = argv[i];
        } else {
            puts(signon);
            fprintf(stderr, "usage: %s [--skip-backups] <file>\n\n", argv[0]);
            return 1;
        }
    }

    if (input_file == NULL) {
        puts(signon);
        fprintf(stderr, "usage: %s [--skip-backups] <file>\n\n", argv[0]);
        return 1;
    }

    HexFile hf;
    memset(&hf, 0, sizeof(hf));

    if (!load_file(&hf, input_file)) {
        free(hf.data);
        return 1;
    }

    puts(signon);

    display_page(&hf, 0);

    char line[4096];

    while (!g_interrupted) {
        printf(": ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            putchar('\n');
            break;
        }

        if (line[0] == '\x1b') {
            break;
        }

        char *p = skip_ws(line);
        char cmd = (*p == '\0' || *p == '\n') ? '\0' : (char)tolower((unsigned char)*p);
        if (cmd != '\0') p++;

        switch (cmd) {
        case '\0':  /* blank line or enter → next page */
            display_page(&hf, hf.view_addr);
            break;

        case 'q':
            goto quit;

        case 'p': {
            size_t start = 0;
            if (hf.view_addr > PAGE_BYTES * 2)
                start = hf.view_addr - PAGE_BYTES * 2;
            display_page(&hf, start);
            break;
        }

        case 'd': {
            p = skip_ws(p);
            size_t addr = 0;
            size_t count = PAGE_BYTES;
            if (*p != '\0' && *p != '\n') {
                char *tok = strtok(p, " \t\r\n");
                if (!parse_hex_addr(tok, &addr)) {
                    fprintf(stderr, "error: invalid address\n");
                    continue;
                }
                char *count_tok = strtok(NULL, " \t\r\n");
                if (count_tok) {
                    if (!parse_hex_addr(count_tok, &count)) {
                        fprintf(stderr, "error: invalid byte count\n");
                        continue;
                    }
                }
            }
            display_count(&hf, addr, count);
            break;
        }

        case 'e':
            do_edit(&hf, p);
            break;

        case 'a': {
            p = skip_ws(p);
            char *end = p + strlen(p);
            while (end > p && isspace((unsigned char)end[-1])) end--;
            *end = '\0';
            do_append(&hf, p);
            break;
        }

        case 'w':
            (void)save_file(&hf, skip_backups);
            break;

        case 's':
            do_search(&hf, p);
            break;

        case 'h':
        case '?':
            print_help();
            break;

        case 'n': {
            p = skip_ws(p);
            char *end = p + strlen(p);
            while (end > p && isspace((unsigned char)end[-1])) end--;
            *end = '\0';
            if (*p == '\0') {
                fprintf(stderr, "error: n requires filename\n");
            } else {
                strncpy(hf.filename, p, sizeof(hf.filename) - 1);
                hf.filename[sizeof(hf.filename) - 1] = '\0';
            }
            break;
        }

        case 'r':
            printf("file: %s\nsize: %zu (0x%zX) bytes\n", hf.filename, hf.size, hf.size);
            break;

        default:
            fprintf(stderr, "error: unknown command '%c'\n", cmd);
            break;
        }
    }
    quit:

    if (g_interrupted) {
        free(hf.data);
        return 0;
    }

    if (!confirm_save_if_needed(&hf, skip_backups)) {
        free(hf.data);
        return 1;
    }

    free(hf.data);
    return 0;
}
