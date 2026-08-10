#ifndef RSS_H
#define RSS_H

#include "parser.h"

typedef struct {
    Post posts[128];
    int  count;
} PostList;

void generate_rss(const PostList *list);

#endif