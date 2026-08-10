#ifndef RENDER_H
#define RENDER_H

#include "parser.h"

char *render_template(const char *tmpl, const Post *post, const char *html_content);
int   render_post(const char *md_path);

#endif