#ifndef TREE_H
#define TREE_H

#include "parser.h"
#include "util.h"

/* The page hierarchy, rebuilt from the slugs collected out of content/.
   A node is a directory, a page, or both: "guide/index.md" gives the "guide"
   node a page of its own, while a directory without one is a bare heading. */
typedef struct TreeNode {
    char  name[MAX_FIELD];   /* this node's URL segment */
    char  path[MAX_PATH];    /* full URL path, "" at the root */
    char  title[MAX_FIELD];
    int   order;
    const Post      *page;   /* NULL for a directory with no index.md */
    struct TreeNode **kids;
    int   nkids;
    int   cap;
} TreeNode;

/* Builds the tree and sorts every level by `order:` then title. Returns NULL
   on allocation failure. The Posts must outlive the tree. */
TreeNode *tree_build(const PostList *pages);
void      tree_free(TreeNode *root);

/* Nested <ul> markup for a sidebar or menu. Directories without a page of
   their own render as a <span> rather than a link. */
int tree_html(const TreeNode *root, StrBuf *out);

/* Depth-first reading order — the sequence the sidebar shows top to bottom,
   which is also the order prev/next follows. Returns how many were written. */
int tree_reading_order(const TreeNode *root, const Post **out, int max);

#endif
