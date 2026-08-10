#include "../include/build.h"
#include "../include/parser.h"
#include "../include/render.h"
#include "../toml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <sys/stat.h>
#endif

extern TomlDoc g_toml;
extern char g_theme_path[512];

void cmd_build(void) {
    DIR *dir = opendir("content/posts");
    if (!dir) {
        fprintf(stderr, "Error: content/posts not found\n");
        fprintf(stderr, "Are you in a project directory? Run atomik-ssg init first.\n");
        return;
    }

    mkdir("public", 0755);
    mkdir("public/posts", 0755);

    char post_items[8192] = {0};
    char item[1024];
    int count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char *ext = strrchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".md") != 0) continue;

        char path[512];
        snprintf(path, sizeof(path), "content/posts/%s", entry->d_name);
        if (render_post(path) == 0) count++;

        char *raw = read_file(path);
        if (!raw) continue;

        Post post = {0};
        if (parse_frontmatter(raw, &post) == 0) {
            snprintf(item, sizeof(item),
                "            <li>\n"
                "                <time>%s</time>\n"
                "                <a href=\"/posts/%s/\">%s</a>\n"
                "                <p>%s</p>\n"
                "            </li>\n",
                post.date, post.slug, post.title, post.description);
            strncat(post_items, item, sizeof(post_items) - strlen(post_items) - 1);
        }
        free(raw);
    }
    closedir(dir);

    char tmpl_path[600];
    snprintf(tmpl_path, sizeof(tmpl_path), "%s/templates/index.html", g_theme_path);
    char *tmpl = read_file(tmpl_path);
    if (!tmpl) { fprintf(stderr, "Error: index template not found\n"); return; }

    Post site = {0};
    strncpy(site.title, toml_get_or(&g_toml, "", "title", "Atomik SSG"), MAX_FIELD - 1);
    strncpy(site.description, toml_get_or(&g_toml, "", "description", ""), MAX_FIELD - 1);

    char *output = render_template(tmpl, &site, "");
    char *pos = strstr(output, "{{post_items}}");
    if (pos) {
        size_t before    = pos - output;
        size_t ilen      = strlen(post_items);
        size_t after_off = before + 14;
        size_t total     = before + ilen + strlen(output + after_off) + 1;
        char *final      = malloc(total);
        memcpy(final, output, before);
        memcpy(final + before, post_items, ilen);
        strcpy(final + before + ilen, output + after_off);

        FILE *f = fopen("public/index.html", "wb");
        if (f) { fputs(final, f); fclose(f); printf("Generated: public/index.html\n"); }
        free(final);
    }
    free(output);
    free(tmpl);

    char static_src[600];
    snprintf(static_src, sizeof(static_src), "%s/static", g_theme_path);
    copy_dir(static_src, "public");
    copy_dir("static", "public");

    printf("\nBuild complete: %d post(s) generated -> public/\n", count);
}