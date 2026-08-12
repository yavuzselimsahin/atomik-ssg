#ifndef RENDER_H
#define RENDER_H

#include "parser.h"

/* One {{name}} substitution. Values are HTML-escaped unless raw is set, which
   is reserved for strings that are already HTML (rendered markdown, the
   generated post list). */
typedef struct {
    const char *name;
    const char *value;
    int         raw;
} TemplateVar;

/* Expands every {{name}} found in vars. Unknown placeholders are left in place
   so a template can carry markup this program does not know about.
   Caller frees the result. */
char *render_template(const char *tmpl, const TemplateVar *vars, int nvars);

/* Renders one entry to outdir[/subdir]/slug/index.html. Pass subdir as "" to
   write directly under outdir (that is what pages do). prev and next may be
   NULL; when they are, the corresponding placeholders expand to nothing.
   tmpl is the already-read template text, so a build reads it only once. */
int render_entry(const char *tmpl, const Post *post,
                 const Post *prev, const Post *next,
                 const char *outdir, const char *subdir);

#endif
