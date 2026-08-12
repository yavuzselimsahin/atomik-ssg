#include "../include/build.h"
#include "../include/parser.h"
#include "../include/render.h"
#include "../include/rss.h"
#include "../include/util.h"
#include "../include/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int append_item(StrBuf *items, const Post *p) {
    char *title = escape_html(p->title);
    char *date  = escape_html(p->date);
    char *desc  = escape_html(p->description);
    int   rc    = -1;

    if (title && date && desc) {
        /* p->slug is already reduced to [a-z0-9-] by collect_posts. */
        rc = sb_appendf(items,
            "            <li>\n"
            "                <time>%s</time>\n"
            "                <a href=\"/posts/%s/\">%s</a>\n"
            "                <p>%s</p>\n"
            "            </li>\n",
            date, p->slug, title, desc);
    }

    free(title);
    free(date);
    free(desc);
    return rc;
}

static int write_index(const PostList *list, const char *items) {
    char tmpl_path[640];
    snprintf(tmpl_path, sizeof(tmpl_path), "%s/templates/index.html", g_theme_path);

    char *tmpl = read_file(tmpl_path);
    if (!tmpl) {
        fprintf(stderr, "Error: index template not found: %s\n", tmpl_path);
        return -1;
    }
    if (!strstr(tmpl, "{{post_items}}"))
        fprintf(stderr, "Warning: %s has no {{post_items}} placeholder, "
                        "the index will not list any posts\n", tmpl_path);

    Post site;
    memset(&site, 0, sizeof(site));
    snprintf(site.title,       sizeof(site.title),       "%s",
             toml_get_or(&g_toml, "", "title", "Atomik SSG"));
    snprintf(site.description, sizeof(site.description), "%s",
             toml_get_or(&g_toml, "", "description", ""));

    char *output = render_template_ex(tmpl, &site, "", items);
    free(tmpl);
    if (!output) return -1;

    char out_path[512];
    snprintf(out_path, sizeof(out_path), "%s/index.html", g_output_dir);

    FILE *f = fopen(out_path, "wb");
    if (!f) { perror(out_path); free(output); return -1; }

    int failed = fputs(output, f) == EOF;
    if (fclose(f) != 0) failed = 1;
    free(output);

    if (failed) { fprintf(stderr, "Error: could not write %s\n", out_path); return -1; }

    printf("Generated: %s (%d post(s) listed)\n", out_path, list->count);
    return 0;
}

void cmd_build(void) {
    PostList list;
    if (collect_posts("content/posts", &list) != 0) {
        fprintf(stderr, "Error: content/posts not found\n");
        fprintf(stderr, "Are you in a project directory? Run atomik-ssg init first.\n");
        return;
    }

    char posts_dir[512];
    snprintf(posts_dir, sizeof(posts_dir), "%s/posts", g_output_dir);
    if (make_dir(g_output_dir) != 0 || make_dir(posts_dir) != 0) {
        perror(g_output_dir);
        postlist_free(&list);
        return;
    }

    StrBuf items;
    sb_init(&items);

    int count = 0;
    for (int i = 0; i < list.count; i++) {
        const Post *p = &list.posts[i];
        if (render_post(p, g_output_dir) != 0) continue;
        if (append_item(&items, p) != 0) {
            fprintf(stderr, "Error: out of memory building the index\n");
            break;
        }
        count++;
    }

    generate_rss(&list, g_output_dir);
    write_index(&list, items.data ? items.data : "");
    sb_free(&items);

    char static_src[640];
    snprintf(static_src, sizeof(static_src), "%s/static", g_theme_path);
    copy_dir(static_src, g_output_dir);
    copy_dir("static", g_output_dir);

    if (count != list.count)
        fprintf(stderr, "Warning: %d of %d post(s) failed to render\n",
                list.count - count, list.count);

    printf("\nBuild complete: %d post(s) generated -> %s/\n", count, g_output_dir);
    postlist_free(&list);
}
