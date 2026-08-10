#include "../include/render.h"
#include "../include/parser.h"
#include "../toml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmark.h>

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <sys/stat.h>
#endif

extern char g_theme_path[512];

char *render_template(const char *tmpl, const Post *post, const char *html_content) {
    size_t out_size = strlen(tmpl) + strlen(html_content) + 4096;
    char *out = malloc(out_size);
    if (!out) return NULL;

    const char *src = tmpl;
    char *dst = out;

    while (*src) {
        if (strncmp(src, "{{title}}", 9) == 0) {
            size_t len = strlen(post->title);
            memcpy(dst, post->title, len);
            dst += len; src += 9;
        } else if (strncmp(src, "{{date}}", 8) == 0) {
            size_t len = strlen(post->date);
            memcpy(dst, post->date, len);
            dst += len; src += 8;
        } else if (strncmp(src, "{{description}}", 15) == 0) {
            size_t len = strlen(post->description);
            memcpy(dst, post->description, len);
            dst += len; src += 15;
        } else if (strncmp(src, "{{site_title}}", 14) == 0) {
            size_t len = strlen(post->title);
            memcpy(dst, post->title, len);
            dst += len; src += 14;
        } else if (strncmp(src, "{{site_description}}", 20) == 0) {
            size_t len = strlen(post->description);
            memcpy(dst, post->description, len);
            dst += len; src += 20;
        } else if (strncmp(src, "{{content}}", 11) == 0) {
            size_t len = strlen(html_content);
            size_t used = dst - out;
            if (used + len + 1024 > out_size) {
                out_size = used + len + 4096;
                char *new_out = realloc(out, out_size);
                if (!new_out) { free(out); return NULL; }
                dst = new_out + used;
                out = new_out;
            }
            memcpy(dst, html_content, len);
            dst += len; src += 11;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
    return out;
}

int render_post(const char *md_path) {
    char *raw = read_file(md_path);
    if (!raw) return -1;

    Post post = {0};
    if (parse_frontmatter(raw, &post) != 0) {
        fprintf(stderr, "Frontmatter error: %s\n", md_path);
        free(raw);
        return -1;
    }

    char *html_content = cmark_markdown_to_html(
        post.content, strlen(post.content), CMARK_OPT_DEFAULT);

    char tmpl_path[600];
    snprintf(tmpl_path, sizeof(tmpl_path), "%s/templates/post.html", g_theme_path);
    char *tmpl = read_file(tmpl_path);
    if (!tmpl) { free(raw); free(html_content); return -1; }

    char *output = render_template(tmpl, &post, html_content);

    char dir[512];
    snprintf(dir, sizeof(dir), "public/posts/%s", post.slug);
    mkdir("public", 0755);
    mkdir("public/posts", 0755);
    mkdir(dir, 0755);

    char out_path[512];
    snprintf(out_path, sizeof(out_path), "%s/index.html", dir);
    FILE *f = fopen(out_path, "wb");
    if (f) {
        fputs(output, f);
        fclose(f);
        printf("Generated: %s\n", out_path);
    }

    free(output);
    free(tmpl);
    free(html_content);
    free(raw);
    return 0;
}