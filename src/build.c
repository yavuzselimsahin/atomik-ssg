#include "../include/build.h"
#include "../include/parser.h"
#include "../include/render.h"
#include "../include/rss.h"
#include "../include/tree.h"
#include "../include/util.h"
#include "../include/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

/* Removes drafts from the list so that everything downstream — the index, the
   feed, prev/next links — needs no special case for them. */
static int drop_drafts(PostList *list) {
    int kept = 0, dropped = 0;

    for (int i = 0; i < list->count; i++) {
        if (list->posts[i].draft && !g_include_drafts) {
            free(list->posts[i].raw);
            dropped++;
        } else {
            list->posts[kept++] = list->posts[i];
        }
    }
    list->count = kept;
    return dropped;
}

static char *read_theme_template(const char *name, int required) {
    char path[640];
    snprintf(path, sizeof(path), "%s/templates/%s", g_theme_path, name);

    FILE *probe = fopen(path, "rb");
    if (!probe) {
        if (required) fprintf(stderr, "Error: template not found: %s\n", path);
        return NULL;
    }
    fclose(probe);
    return read_file(path);
}

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

static int write_index(int listed, const char *items) {
    char *tmpl = read_theme_template("index.html", 1);
    if (!tmpl) return -1;

    /* The docs theme leaves this out on purpose, so only complain when there
       are posts that would otherwise go unlisted. */
    if (listed > 0 && !strstr(tmpl, "{{post_items}}"))
        fprintf(stderr, "Warning: the index template has no {{post_items}} "
                        "placeholder, so its %d post(s) will not be listed\n", listed);

    const TemplateVar vars[] = {
        { "title",            g_site_title,       0 },
        { "site_title",       g_site_title,       0 },
        { "description",      g_site_description, 0 },
        { "site_description", g_site_description, 0 },
        { "post_items",       items,              1 },
        { "pages",            g_nav_html,         1 },
        { "page_tree",        g_tree_html,        1 },
    };

    char *output = render_template(tmpl, vars, (int)(sizeof(vars) / sizeof(vars[0])));
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

    printf("Generated: %s (%d post(s) listed)\n", out_path, listed);
    return 0;
}

/* The menu is derived from what is in content/, so adding a page is enough to
   publish it — there is no list to keep in sync. Only the top level goes in
   the menu bar; the full hierarchy is available separately as {{page_tree}}. */
static int build_nav(const TreeNode *tree, StrBuf *nav) {
    for (int i = 0; i < tree->nkids; i++) {
        const TreeNode *k = tree->kids[i];
        if (!k->page || strcmp(k->path, "posts") == 0) continue;

        char *title = escape_html(k->title);
        if (!title) return -1;

        int rc = sb_appendf(nav, "<li><a href=\"/%s/\">%s</a></li>", k->path, title);
        free(title);
        if (rc != 0) return -1;
    }
    return 0;
}

/* Markdown under content/ becomes a standalone page at /<slug>/. They are
   rendered in reading order so that prev/next walks the sidebar rather than a
   calendar — documentation has a sequence, and that sequence is the tree. */
static int render_pages(const PostList *pages, const TreeNode *tree) {
    if (pages->count == 0) return 0;

    /* A theme need not ship page.html; post.html is a reasonable stand-in. */
    char *tmpl = read_theme_template("page.html", 0);
    if (!tmpl) tmpl = read_theme_template("post.html", 1);
    if (!tmpl) return 0;

    const Post **order = malloc((size_t)pages->count * sizeof(*order));
    if (!order) { free(tmpl); return 0; }
    int n = tree_reading_order(tree, order, pages->count);

    int count = 0;
    for (int i = 0; i < n; i++) {
        const Post *p    = order[i];
        const Post *prev = (i > 0)     ? order[i - 1] : NULL;
        const Post *next = (i + 1 < n) ? order[i + 1] : NULL;

        if (strcmp(p->slug, "posts") == 0) {
            fprintf(stderr, "Warning: page slug \"posts\" collides with the post "
                            "directory, skipping it\n");
            continue;
        }
        if (render_entry(tmpl, p, prev, next, g_output_dir, "") == 0) count++;
    }

    free(order);
    free(tmpl);
    return count;
}

void cmd_build(void) {
    /* content/posts is optional: a documentation site need not carry a blog.
       Only a missing content/ means we are not in a project at all. */
    PostList list;
    if (collect_posts("content/posts", &list) != 0) {
        memset(&list, 0, sizeof(list));

        DIR *content = opendir("content");
        if (!content) {
            fprintf(stderr, "Error: content/ not found\n");
            fprintf(stderr, "Are you in a project directory? Run atomik-ssg init first.\n");
            return;
        }
        closedir(content);
    }

    int drafts = drop_drafts(&list);

    char posts_dir[512];
    snprintf(posts_dir, sizeof(posts_dir), "%s/posts", g_output_dir);
    if (make_dir(g_output_dir) != 0 || make_dir(posts_dir) != 0) {
        perror(g_output_dir);
        postlist_free(&list);
        return;
    }

    /* Pages are collected and arranged before anything renders: the menu and
       the sidebar they produce have to appear on posts and the index too. */
    PostList  pages;
    TreeNode *tree = NULL;
    StrBuf    nav, treebuf;
    sb_init(&nav);
    sb_init(&treebuf);

    if (collect_pages("content", &pages, "posts") == 0) {
        drafts += drop_drafts(&pages);

        tree = tree_build(&pages);
        if (!tree) {
            fprintf(stderr, "Warning: out of memory building the page tree\n");
        } else {
            if (build_nav(tree, &nav) != 0)
                fprintf(stderr, "Warning: out of memory building the page menu\n");
            if (tree_html(tree, &treebuf) != 0)
                fprintf(stderr, "Warning: out of memory building the page tree\n");
            if (nav.data)     g_nav_html  = nav.data;
            if (treebuf.data) g_tree_html = treebuf.data;
        }
    } else {
        memset(&pages, 0, sizeof(pages));
    }

    /* Read once rather than per post. */
    char *post_tmpl = read_theme_template("post.html", 1);
    if (!post_tmpl) { postlist_free(&list); return; }

    StrBuf items;
    sb_init(&items);

    int count = 0;
    for (int i = 0; i < list.count; i++) {
        const Post *p = &list.posts[i];
        /* Sorted newest first, so the older neighbour is the next element. */
        const Post *prev = (i + 1 < list.count) ? &list.posts[i + 1] : NULL;
        const Post *next = (i > 0)              ? &list.posts[i - 1] : NULL;

        if (render_entry(post_tmpl, p, prev, next, g_output_dir, "posts") != 0) continue;
        if (append_item(&items, p) != 0) {
            fprintf(stderr, "Error: out of memory building the index\n");
            break;
        }
        count++;
    }
    free(post_tmpl);

    int page_count = tree ? render_pages(&pages, tree) : 0;

    generate_rss(&list, g_output_dir);
    write_index(count, items.data ? items.data : "");
    sb_free(&items);

    char static_src[640];
    snprintf(static_src, sizeof(static_src), "%s/static", g_theme_path);
    copy_dir(static_src, g_output_dir);
    copy_dir("static", g_output_dir);

    if (count != list.count)
        fprintf(stderr, "Warning: %d of %d post(s) failed to render\n",
                list.count - count, list.count);

    printf("\nBuild complete: %d post(s)", count);
    if (page_count) printf(", %d page(s)", page_count);
    if (drafts)     printf(", %d draft(s) skipped", drafts);
    printf(" -> %s/\n", g_output_dir);

    if (drafts && !g_include_drafts)
        printf("Run with --drafts to include them.\n");

    g_nav_html  = "";
    g_tree_html = "";
    tree_free(tree);
    sb_free(&nav);
    sb_free(&treebuf);
    postlist_free(&pages);
    postlist_free(&list);
}
