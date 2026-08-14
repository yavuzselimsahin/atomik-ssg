#include "../include/tree.h"
#include "../include/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static TreeNode *node_new(const char *name, const char *path) {
    TreeNode *n = calloc(1, sizeof(TreeNode));
    if (!n) return NULL;

    snprintf(n->name,  sizeof(n->name),  "%s", name);
    snprintf(n->path,  sizeof(n->path),  "%s", path);
    snprintf(n->title, sizeof(n->title), "%s", name);
    n->order = INT_MAX;
    return n;
}

static TreeNode *node_child(TreeNode *parent, const char *name, const char *path) {
    for (int i = 0; i < parent->nkids; i++)
        if (strcmp(parent->kids[i]->name, name) == 0) return parent->kids[i];

    if (parent->nkids == parent->cap) {
        int        ncap = parent->cap ? parent->cap * 2 : 8;
        TreeNode **nk   = realloc(parent->kids, (size_t)ncap * sizeof(TreeNode *));
        if (!nk) return NULL;
        parent->kids = nk;
        parent->cap  = ncap;
    }

    TreeNode *n = node_new(name, path);
    if (!n) return NULL;
    parent->kids[parent->nkids++] = n;
    return n;
}

/* Same rule as the page menu: an explicit order wins, then titles compared
   after transliteration so a non-ASCII title is not exiled to the bottom. */
static int node_cmp(const void *a, const void *b) {
    const TreeNode *na = *(const TreeNode **)a;
    const TreeNode *nb = *(const TreeNode **)b;

    if (na->order != nb->order) return na->order < nb->order ? -1 : 1;

    char ka[MAX_FIELD], kb[MAX_FIELD];
    slugify(na->title, ka, sizeof(ka));
    slugify(nb->title, kb, sizeof(kb));

    int c = strcmp(ka, kb);
    return c ? c : strcmp(na->name, nb->name);
}

static void sort_recursive(TreeNode *n) {
    if (n->nkids > 1)
        qsort(n->kids, (size_t)n->nkids, sizeof(TreeNode *), node_cmp);
    for (int i = 0; i < n->nkids; i++)
        sort_recursive(n->kids[i]);
}

TreeNode *tree_build(const PostList *pages) {
    TreeNode *root = node_new("", "");
    if (!root) return NULL;

    for (int i = 0; i < pages->count; i++) {
        const Post *p = &pages->posts[i];

        char segs[MAX_PATH];
        snprintf(segs, sizeof(segs), "%s", p->slug);

        TreeNode *cur  = root;
        char      path[MAX_PATH] = "";
        int       ok   = 1;

        for (char *tok = strtok(segs, "/"); tok && ok; tok = strtok(NULL, "/")) {
            size_t len = strlen(path);
            snprintf(path + len, sizeof(path) - len, "%s%s", len ? "/" : "", tok);

            cur = node_child(cur, tok, path);
            if (!cur) ok = 0;
        }

        if (!ok) { tree_free(root); return NULL; }

        /* Reaching a node that already has a page means two files claim the
           same URL; the first one collected keeps it. */
        if (cur == root) continue;
        if (cur->page) {
            fprintf(stderr, "Warning: %s and %s both map to /%s/, keeping the first\n",
                    cur->page->source, p->source, cur->path);
            continue;
        }

        cur->page  = p;
        cur->order = p->order;
        snprintf(cur->title, sizeof(cur->title), "%s", p->title);
    }

    sort_recursive(root);
    return root;
}

void tree_free(TreeNode *root) {
    if (!root) return;
    for (int i = 0; i < root->nkids; i++)
        tree_free(root->kids[i]);
    free(root->kids);
    free(root);
}

static int emit(const TreeNode *n, StrBuf *out) {
    if (n->nkids == 0) return 0;
    if (sb_append(out, "<ul>") != 0) return -1;

    for (int i = 0; i < n->nkids; i++) {
        const TreeNode *k = n->kids[i];

        char *title = escape_html(k->title);
        if (!title) return -1;

        int rc = k->page
            ? sb_appendf(out, "<li><a href=\"%s/%s/\">%s</a>", g_base_path, k->path, title)
            : sb_appendf(out, "<li><span>%s</span>", title);
        free(title);
        if (rc != 0) return -1;

        if (emit(k, out) != 0) return -1;
        if (sb_append(out, "</li>") != 0) return -1;
    }

    return sb_append(out, "</ul>");
}

int tree_html(const TreeNode *root, StrBuf *out) {
    return emit(root, out);
}

static int flatten(const TreeNode *n, const Post **out, int max, int count) {
    for (int i = 0; i < n->nkids && count < max; i++) {
        const TreeNode *k = n->kids[i];
        if (k->page && count < max) out[count++] = k->page;
        count = flatten(k, out, max, count);
    }
    return count;
}

int tree_reading_order(const TreeNode *root, const Post **out, int max) {
    return flatten(root, out, max, 0);
}
