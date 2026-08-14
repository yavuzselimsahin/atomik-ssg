#include "../include/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#ifdef _WIN32
    #define strcasecmp _stricmp
#else
    #include <strings.h>
#endif

#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
#endif

#define SB_MIN_CAP 256

void sb_init(StrBuf *sb) {
    sb->data = NULL;
    sb->len  = 0;
    sb->cap  = 0;
}

void sb_free(StrBuf *sb) {
    free(sb->data);
    sb->data = NULL;
    sb->len  = 0;
    sb->cap  = 0;
}

int sb_reserve(StrBuf *sb, size_t extra) {
    size_t need = sb->len + extra + 1;
    if (need <= sb->cap) return 0;

    size_t cap = sb->cap ? sb->cap : SB_MIN_CAP;
    while (cap < need) {
        if (cap > ((size_t)-1) / 2) return -1;
        cap *= 2;
    }
    char *p = realloc(sb->data, cap);
    if (!p) return -1;
    sb->data = p;
    sb->cap  = cap;
    return 0;
}

int sb_append_n(StrBuf *sb, const char *s, size_t n) {
    if (sb_reserve(sb, n) != 0) return -1;
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
    return 0;
}

int sb_append(StrBuf *sb, const char *s) {
    if (!s) return 0;
    return sb_append_n(sb, s, strlen(s));
}

int sb_appendf(StrBuf *sb, const char *fmt, ...) {
    va_list ap, ap2;
    va_start(ap, fmt);
    va_copy(ap2, ap);

    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0) { va_end(ap2); return -1; }

    if (sb_reserve(sb, (size_t)n) != 0) { va_end(ap2); return -1; }
    vsnprintf(sb->data + sb->len, (size_t)n + 1, fmt, ap2);
    va_end(ap2);

    sb->len += (size_t)n;
    return 0;
}

int sb_append_escaped(StrBuf *sb, const char *s) {
    if (!s) return 0;
    for (; *s; s++) {
        const char *rep = NULL;
        switch (*s) {
            case '&':  rep = "&amp;";  break;
            case '<':  rep = "&lt;";   break;
            case '>':  rep = "&gt;";   break;
            case '"':  rep = "&quot;"; break;
            case '\'': rep = "&#39;";  break;
            default:   break;
        }
        int rc = rep ? sb_append(sb, rep) : sb_append_n(sb, s, 1);
        if (rc != 0) return -1;
    }
    return 0;
}

char *escape_html(const char *s) {
    StrBuf sb;
    sb_init(&sb);
    if (sb_append_escaped(&sb, s) != 0) { sb_free(&sb); return NULL; }
    if (!sb.data && sb_append_n(&sb, "", 0) != 0) return NULL;
    return sb.data;
}

/* ------------------------------------------------------------------ */

typedef struct {
    const char *utf8;
    const char *ascii;
} Translit;

/* Longest sequences first is not required here: every entry is a distinct
   2-byte UTF-8 sequence, so first match wins unambiguously. */
static const Translit TRANSLIT[] = {
    /* Turkish */
    {"ç", "c"}, {"Ç", "c"}, {"ğ", "g"}, {"Ğ", "g"}, {"ı", "i"}, {"İ", "i"},
    {"ö", "o"}, {"Ö", "o"}, {"ş", "s"}, {"Ş", "s"}, {"ü", "u"}, {"Ü", "u"},
    /* Latin-1 and friends */
    {"á", "a"}, {"à", "a"}, {"â", "a"}, {"ä", "a"}, {"ã", "a"}, {"å", "a"},
    {"Á", "a"}, {"À", "a"}, {"Â", "a"}, {"Ä", "a"}, {"Ã", "a"}, {"Å", "a"},
    {"é", "e"}, {"è", "e"}, {"ê", "e"}, {"ë", "e"},
    {"É", "e"}, {"È", "e"}, {"Ê", "e"}, {"Ë", "e"},
    {"í", "i"}, {"ì", "i"}, {"î", "i"}, {"ï", "i"},
    {"Í", "i"}, {"Ì", "i"}, {"Î", "i"}, {"Ï", "i"},
    {"ó", "o"}, {"ò", "o"}, {"ô", "o"}, {"õ", "o"},
    {"Ó", "o"}, {"Ò", "o"}, {"Ô", "o"}, {"Õ", "o"},
    {"ú", "u"}, {"ù", "u"}, {"û", "u"},
    {"Ú", "u"}, {"Ù", "u"}, {"Û", "u"},
    {"ñ", "n"}, {"Ñ", "n"}, {"ý", "y"}, {"ÿ", "y"},
    {"ß", "ss"}, {"æ", "ae"}, {"Æ", "ae"}, {"ø", "o"}, {"Ø", "o"},
    {"č", "c"}, {"Č", "c"}, {"š", "s"}, {"Š", "s"}, {"ž", "z"}, {"Ž", "z"},
    {"ł", "l"}, {"Ł", "l"}, {"ć", "c"}, {"Ć", "c"}, {"đ", "d"}, {"Đ", "d"},
    {NULL, NULL}
};

void slugify(const char *in, char *out, size_t out_size) {
    if (out_size == 0) return;
    out[0] = '\0';
    if (!in) return;

    size_t j = 0;
    int pending_dash = 0;   /* a separator is owed, emitted lazily */
    int have_alnum   = 0;   /* suppresses leading dashes */

    for (size_t i = 0; in[i] && j + 1 < out_size; ) {
        unsigned char c = (unsigned char)in[i];

        if (c < 0x80) {
            char o = 0;
            if (c >= 'A' && c <= 'Z')                          o = (char)(c + 32);
            else if ((c >= 'a' && c <= 'z') || isdigit(c))      o = (char)c;

            if (o) {
                if (pending_dash && have_alnum && j + 1 < out_size) out[j++] = '-';
                pending_dash = 0;
                if (j + 1 < out_size) { out[j++] = o; have_alnum = 1; }
            } else {
                pending_dash = 1;
            }
            i++;
            continue;
        }

        /* Multi-byte sequence: transliterate if known, else treat as a break. */
        const char *rep   = NULL;
        size_t      mblen = 1;
        for (const Translit *t = TRANSLIT; t->utf8; t++) {
            size_t tl = strlen(t->utf8);
            if (strncmp(in + i, t->utf8, tl) == 0) { rep = t->ascii; mblen = tl; break; }
        }

        if (rep) {
            if (pending_dash && have_alnum && j + 1 < out_size) out[j++] = '-';
            pending_dash = 0;
            for (const char *r = rep; *r && j + 1 < out_size; r++) { out[j++] = *r; have_alnum = 1; }
        } else {
            while (in[i + mblen] && ((unsigned char)in[i + mblen] & 0xC0) == 0x80) mblen++;
            pending_dash = 1;
        }
        i += mblen;
    }

    out[j] = '\0';
}

int rfc822_date(const char *iso, char *out, size_t out_size) {
    static const char *WDAY[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char *MON[]  = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    int y, m, d;

    if (!iso || sscanf(iso, "%d-%d-%d", &y, &m, &d) != 3) return -1;
    if (m < 1 || m > 12 || d < 1 || d > 31 || y < 1900) return -1;

    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year  = y - 1900;
    tm.tm_mon   = m - 1;
    tm.tm_mday  = d;
    tm.tm_hour  = 12;      /* midday keeps DST shifts from moving the date */
    tm.tm_isdst = -1;

    if (mktime(&tm) == (time_t)-1) return -1;
    if (tm.tm_wday < 0 || tm.tm_wday > 6) return -1;

    snprintf(out, out_size, "%s, %02d %s %04d 00:00:00 +0000",
             WDAY[tm.tm_wday], d, MON[m - 1], y);
    return 0;
}

char *shell_quote(const char *s) {
    StrBuf sb;
    sb_init(&sb);
    if (!s) s = "";

    if (sb_append(&sb, "'") != 0) { sb_free(&sb); return NULL; }
    for (; *s; s++) {
        int rc = (*s == '\'') ? sb_append(&sb, "'\\''") : sb_append_n(&sb, s, 1);
        if (rc != 0) { sb_free(&sb); return NULL; }
    }
    if (sb_append(&sb, "'") != 0) { sb_free(&sb); return NULL; }
    return sb.data;
}

static const char *CALLOUT_KINDS[] = {
    "NOTE", "TIP", "IMPORTANT", "WARNING", "CAUTION", NULL
};

char *mark_callouts(const char *html) {
    static const char OPEN[] = "<blockquote>\n<p>[!";
    const size_t      OPEN_LEN = sizeof(OPEN) - 1;

    StrBuf sb;
    sb_init(&sb);
    if (!html) html = "";

    for (const char *p = html; *p; ) {
        if (strncmp(p, OPEN, OPEN_LEN) == 0) {
            const char *kind = p + OPEN_LEN;
            const char *end  = strchr(kind, ']');

            if (end && (size_t)(end - kind) < 16) {
                char name[16];
                size_t n = (size_t)(end - kind);
                memcpy(name, kind, n);
                name[n] = '\0';

                int known = 0;
                for (int i = 0; CALLOUT_KINDS[i]; i++)
                    if (strcmp(name, CALLOUT_KINDS[i]) == 0) known = 1;

                if (known) {
                    for (size_t i = 0; i < n; i++)
                        name[i] = (char)tolower((unsigned char)name[i]);

                    if (sb_appendf(&sb, "<blockquote class=\"callout callout-%s\">\n<p>",
                                   name) != 0) { sb_free(&sb); return NULL; }

                    p = end + 1;
                    /* The marker sat either on its own line or on the first
                       line of the text; both leave something to step over. */
                    if      (strncmp(p, "</p>\n<p>", 8) == 0) p += 8;
                    else if (*p == '\n')                      p += 1;
                    continue;
                }
            }
        }
        if (sb_append_n(&sb, p, 1) != 0) { sb_free(&sb); return NULL; }
        p++;
    }

    if (!sb.data && sb_append_n(&sb, "", 0) != 0) return NULL;
    return sb.data;
}

char *prefix_links(const char *html, const char *prefix) {
    StrBuf sb;
    sb_init(&sb);
    if (!html) html = "";

    for (const char *p = html; *p; ) {
        /* cmark escapes quotes inside code, so a bare href="/ is always a
           real attribute and never something the author was quoting. */
        size_t alen = 0;
        if      (strncmp(p, "href=\"/", 7) == 0) alen = 7;
        else if (strncmp(p, "src=\"/",  6) == 0) alen = 6;

        if (alen && p[alen] != '/') {          /* "//host" is another site */
            if (sb_append_n(&sb, p, alen - 1) != 0) { sb_free(&sb); return NULL; }
            if (sb_append(&sb, prefix) != 0)        { sb_free(&sb); return NULL; }
            p += alen - 1;                      /* the '/' still has to be written */
            continue;
        }
        if (sb_append_n(&sb, p, 1) != 0) { sb_free(&sb); return NULL; }
        p++;
    }

    if (!sb.data && sb_append_n(&sb, "", 0) != 0) return NULL;
    return sb.data;
}

int is_truthy(const char *v) {
    if (!v) return 0;
    return strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 ||
           strcasecmp(v, "on")   == 0 || strcmp(v, "1") == 0;
}

int make_dir(const char *path) {
#ifdef _WIN32
    if (_mkdir(path) == 0) return 0;
#else
    if (mkdir(path, 0755) == 0) return 0;
#endif
    return (errno == EEXIST) ? 0 : -1;
}

int make_dir_p(const char *path) {
    char tmp[1024];
    if (snprintf(tmp, sizeof(tmp), "%s", path) >= (int)sizeof(tmp)) return -1;

    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        if (make_dir(tmp) != 0) return -1;
        *p = '/';
    }
    return make_dir(tmp);
}
