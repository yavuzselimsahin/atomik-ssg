#include "../include/render.h"
#include "../include/parser.h"
#include "../include/util.h"
#include "../include/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmark.h>

#define MAX_VAR_NAME 32

char *render_template(const char *tmpl, const TemplateVar *vars, int nvars) {
    StrBuf sb;
    sb_init(&sb);

    const char *src = tmpl;
    int         ok  = 1;

    while (*src && ok) {
        if (src[0] == '{' && src[1] == '{') {
            const char *end = strstr(src + 2, "}}");
            if (end) {
                size_t nlen = (size_t)(end - (src + 2));
                if (nlen > 0 && nlen < MAX_VAR_NAME) {
                    char name[MAX_VAR_NAME];
                    memcpy(name, src + 2, nlen);
                    name[nlen] = '\0';

                    const TemplateVar *hit = NULL;
                    for (int i = 0; i < nvars; i++) {
                        if (strcmp(vars[i].name, name) == 0) { hit = &vars[i]; break; }
                    }

                    if (hit) {
                        const char *v = hit->value ? hit->value : "";
                        ok  = (hit->raw ? sb_append(&sb, v) : sb_append_escaped(&sb, v)) == 0;
                        src = end + 2;
                        continue;
                    }
                }
            }
        }
        ok = sb_append_n(&sb, src, 1) == 0;
        src++;
    }

    if (!ok) {
        sb_free(&sb);
        fprintf(stderr, "Error: out of memory rendering template\n");
        return NULL;
    }
    if (!sb.data && sb_append_n(&sb, "", 0) != 0) return NULL;
    return sb.data;
}

int render_entry(const char *tmpl, const Post *post,
                 const Post *prev, const Post *next,
                 const char *outdir, const char *subdir) {
    const char *body = post->content ? post->content : "";

    char *html = cmark_markdown_to_html(body, strlen(body), CMARK_OPT_DEFAULT);
    if (!html) {
        fprintf(stderr, "Error: markdown conversion failed for %s\n", post->slug);
        return -1;
    }

    /* Neighbour URLs are built here so templates stay free of path logic. */
    char prev_url[MAX_FIELD + 32] = "";
    char next_url[MAX_FIELD + 32] = "";
    if (prev) snprintf(prev_url, sizeof(prev_url), "/%s%s%s/", subdir, *subdir ? "/" : "", prev->slug);
    if (next) snprintf(next_url, sizeof(next_url), "/%s%s%s/", subdir, *subdir ? "/" : "", next->slug);

    const TemplateVar vars[] = {
        { "title",             post->title,               0 },
        { "date",              post->date,                0 },
        { "description",       post->description,         0 },
        { "slug",              post->slug,                0 },
        { "site_title",        g_site_title,              0 },
        { "site_description",  g_site_description,        0 },
        { "content",           html,                      1 },
        { "pages",             g_nav_html,                1 },
        { "prev_url",          prev_url,                  0 },
        { "prev_title",        prev ? prev->title : "",   0 },
        { "next_url",          next_url,                  0 },
        { "next_title",        next ? next->title : "",   0 },
    };

    char *output = render_template(tmpl, vars, (int)(sizeof(vars) / sizeof(vars[0])));
    free(html);
    if (!output) return -1;

    char dir[1024];
    int  n = *subdir
        ? snprintf(dir, sizeof(dir), "%s/%s/%s", outdir, subdir, post->slug)
        : snprintf(dir, sizeof(dir), "%s/%s", outdir, post->slug);

    if (n < 0 || n >= (int)sizeof(dir)) {
        fprintf(stderr, "Error: output path too long for %s\n", post->slug);
        free(output);
        return -1;
    }
    if (make_dir(dir) != 0) {
        perror(dir);
        free(output);
        return -1;
    }

    char out_path[1040];
    snprintf(out_path, sizeof(out_path), "%s/index.html", dir);

    FILE *f = fopen(out_path, "wb");
    if (!f) { perror(out_path); free(output); return -1; }

    int failed = fputs(output, f) == EOF;
    if (fclose(f) != 0) failed = 1;
    free(output);

    if (failed) {
        fprintf(stderr, "Error: could not write %s\n", out_path);
        return -1;
    }
    printf("Generated: %s\n", out_path);
    return 0;
}
