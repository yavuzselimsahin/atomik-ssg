#ifndef RENDER_H
#define RENDER_H

#include "parser.h"

/* Expands {{title}}, {{date}}, {{description}}, {{slug}}, {{site_title}},
   {{site_description}} (all HTML-escaped), plus {{content}} and
   {{post_items}} (inserted verbatim — callers pass ready-made HTML).
   Unknown placeholders are left untouched. Caller frees the result. */
char *render_template_ex(const char *tmpl, const Post *post,
                         const char *html_content, const char *post_items);
char *render_template(const char *tmpl, const Post *post, const char *html_content);

int render_post(const Post *post, const char *outdir);

#endif
