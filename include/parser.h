#ifndef PARSER_H
#define PARSER_H

#define MAX_LINE  512
#define MAX_FIELD 256

typedef struct {
    char  title[MAX_FIELD];
    char  date[MAX_FIELD];
    char  slug[MAX_FIELD];
    char  description[MAX_FIELD];
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
void  postlist_free(PostList *list);

#endif
