#ifndef PARSER_H
#define PARSER_H

#define MAX_LINE  512
#define MAX_FIELD 256
#define MAX_PATH  512

typedef struct {
    char  title[MAX_FIELD];
    char  date[MAX_FIELD];
    /* For a post this is one segment; for a page it is the URL path, which may
       be nested ("guide/install"). Every segment is reduced to [a-z0-9-]. */
    char  slug[MAX_PATH];
    char  description[MAX_FIELD];
    char  source[MAX_PATH];   /* the markdown file this came from */
    int   draft;     /* frontmatter `draft: true` */
    int   order;     /* frontmatter `order: N`, orders menus and the sidebar */
    char *content;   /* points into raw, never freed on its own */
    char *raw;       /* owned by the Post; released by postlist_free() */
} Post;

typedef struct {
    Post *posts;
    int   count;
    int   cap;
} PostList;

int   parse_frontmatter(char *raw, Post *post);
char *read_file(const char *path);
void  copy_file(const char *src, const char *dst);
void  copy_dir(const char *src, const char *dst);

/* Reads every *.md in dir, fills in missing slug/date/title from the file
   name, and sorts newest first. Returns -1 if dir cannot be opened. */
int   collect_posts(const char *dir, PostList *list);

/* Same, but walks subdirectories: content/guide/install.md becomes the page
   "guide/install". A directory's index.md becomes the directory itself
   ("guide/index.md" -> "guide"). skip_subdir, when given, is ignored at the
   top level — that is how content/posts/ stays out of the page tree.
   The result is unsorted; the caller arranges it. */
int   collect_pages(const char *dir, PostList *list, const char *skip_subdir);

void  postlist_free(PostList *list);

#endif
