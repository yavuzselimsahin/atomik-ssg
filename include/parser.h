#ifndef PARSER_H
#define PARSER_H

#define MAX_LINE  512
#define MAX_FIELD 256

typedef struct {
    char title[MAX_FIELD];
    char date[MAX_FIELD];
    char slug[MAX_FIELD];
    char description[MAX_FIELD];
    char *content;
} Post;

int   parse_frontmatter(const char *raw, Post *post);
char *read_file(const char *path);
void  copy_file(const char *src, const char *dst);
void  copy_dir(const char *src, const char *dst);

#endif