#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/* Growable string buffer. Zero-initialised via sb_init(). */
typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} StrBuf;

void sb_init(StrBuf *sb);
void sb_free(StrBuf *sb);
int  sb_reserve(StrBuf *sb, size_t extra);
int  sb_append_n(StrBuf *sb, const char *s, size_t n);
int  sb_append(StrBuf *sb, const char *s);
int  sb_appendf(StrBuf *sb, const char *fmt, ...);

/* Appends s with &<>"' replaced by entities. Safe for both HTML and XML. */
int   sb_append_escaped(StrBuf *sb, const char *s);
/* Same, as a freshly allocated string. Never returns NULL except on OOM. */
char *escape_html(const char *s);

/* ASCII slug: lowercases, transliterates Turkish/Latin accents, collapses
   every other run of characters into a single '-'. Always NUL-terminates. */
void slugify(const char *in, char *out, size_t out_size);

/* "2026-08-10" -> "Mon, 10 Aug 2026 00:00:00 +0000". Returns -1 if unparsable. */
int rfc822_date(const char *iso, char *out, size_t out_size);

/* Wraps s in single quotes so a shell treats it as one literal argument. */
char *shell_quote(const char *s);

/* Turns GitHub's alert syntax into a classed blockquote:

       > [!WARNING]              <blockquote class="callout callout-warning">
       > mind this        ->     <p>mind this</p></blockquote>

   The label itself comes from CSS, so a theme decides how it reads. Only the
   five known kinds are touched; anything else stays an ordinary quote.
   Caller frees. */
char *mark_callouts(const char *html);

/* Returns a copy of html with prefix inserted in front of every root-absolute
   href/src. Protocol-relative "//host" URLs are left alone. Caller frees. */
char *prefix_links(const char *html, const char *prefix);

/* Accepts the spellings people actually write: true, yes, on, 1. */
int is_truthy(const char *v);

/* mkdir() that succeeds if the directory already exists. */
int make_dir(const char *path);

/* Creates every missing component of path, like `mkdir -p`. */
int make_dir_p(const char *path);

#endif
