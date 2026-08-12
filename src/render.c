#include "../include/render.h"
#include "../include/parser.h"
#include "../include/util.h"
#include "../include/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmark.h>

char *render_template_ex(const char *tmpl, const Post *post,
                         const char *html_content, const char *post_items) {
    StrBuf sb;
    sb_init(&sb);

    if (!html_content) html_content = "";
    const char *src = tmpl;
    int ok = 1;

    while (*src && ok) {
        if (src[0] == '{' && src[1] == '{') {
            const char *end = strstr(src + 2, "}}");
            if (end) {
                size_t nlen = (size_t)(end - (src + 2));
                char   name[64];

                if (nlen < sizeof(name)) {
                    memcpy(name, src + 2, nlen);
                    name[nlen] = '\0';

                    const char *escaped = NULL;
                    const char *verbatim = NULL;

                    if      (strcmp(name, "title") == 0 ||
                             strcmp(name, "site_title") == 0)       escaped = post->title;
                    else if (strcmp(name, "description") == 0 ||
                             strcmp(name, "site_description") == 0) escaped = post->description;
                    else if (strcmp(name, "date") == 0)             escaped = post->date;
                    else if (strcmp(name, "slug") == 0)             escaped = post->slug;
                    else if (strcmp(name, "content") == 0)          verbatim = html_content;
                    else if (strcmp(name, "post_items") == 0 && post_items) verbatim = post_items;

                    if (escaped) {
                        ok = sb_append_escaped(&sb, escaped) == 0;
                        src = end + 2;
                        continue;
                    }
                    if (verbatim) {
                        ok = sb_append(&sb, verbatim) == 0;
                        src = end + 2;
                        continue;
                    }
                }
            }
        }
        ok = sb_append_n(&sb, src, 1) == 0;
        src++;
    }

    if (!ok) { sb_free(&sb); fprintf(stderr, "Error: out of memory rendering template\n"); return NULL; }
    if (!sb.data && sb_append_n(&sb, "", 0) != 0) return NULL;
    return sb.data;
}

char *render_template(const char *tmpl, const Post *post, const char *html_content) {
    return render_template_ex(tmpl, post, html_content, NULL);
}

int render_post(const Post *post, const char *outdir) {
    const char *body = post->content ? post->content : "";

    char *html_content = cmark_markdown_to_html(body, strlen(body), CMARK_OPT_DEFAULT);
    if (!html_content) {
        fprintf(stderr, "Error: markdown conversion failed for %s\n", post->slug);
        return -1;
    }

    char tmpl_path[640];
    snprintf(tmpl_path, sizeof(tmpl_path), "%s/templates/post.html", g_theme_path);
    char *tmpl = read_file(tmpl_path);
    if (!tmpl) { free(html_content); return -1; }

    char *output = render_template(tmpl, post, html_content);
    free(tmpl);
    free(html_content);
    if (!output) return -1;

    char dir[1024];
    if (snprintf(dir, sizeof(dir), "%s/posts/%s", outdir, post->slug) >= (int)sizeof(dir)) {
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
