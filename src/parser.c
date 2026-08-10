#include "../include/parser.h"
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

int parse_frontmatter(const char *raw, Post *post) {
    if (strncmp(raw, "---", 3) != 0) return -1;
    const char *ptr = raw + 3;
    while (*ptr == '\r' || *ptr == '\n') ptr++;

    char line[MAX_LINE];
    while (*ptr) {
        int i = 0;
        while (*ptr && *ptr != '\n' && i < MAX_LINE - 1)
            line[i++] = *ptr++;
        line[i] = '\0';
        if (*ptr == '\n') ptr++;

        int llen = strlen(line);
        if (llen > 0 && line[llen-1] == '\r') line[llen-1] = '\0';

        if (strncmp(line, "---", 3) == 0) {
            while (*ptr == '\r' || *ptr == '\n') ptr++;
            post->content = (char *)ptr;
            return 0;
        }

        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = '\0';
        char *key = line;
        char *val = colon + 1;
        while (*val == ' ') val++;

        char *cr = strchr(val, '\r');
        if (cr) *cr = '\0';

        if (strcmp(key, "title") == 0)
            strncpy(post->title, val, MAX_FIELD - 1);
        else if (strcmp(key, "date") == 0)
            strncpy(post->date, val, MAX_FIELD - 1);
        else if (strcmp(key, "slug") == 0)
            strncpy(post->slug, val, MAX_FIELD - 1);
        else if (strcmp(key, "description") == 0)
            strncpy(post->description, val, MAX_FIELD - 1);
    }
    return -1;
}

char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return NULL; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *buf = malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

void copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) { fprintf(stderr, "Warning: cannot open %s\n", src); return; }
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); fprintf(stderr, "Warning: cannot write %s\n", dst); return; }
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        fwrite(buf, 1, n, out);
    fclose(in);
    fclose(out);
}

void copy_dir(const char *src, const char *dst) {
    DIR *d = opendir(src);
    if (!d) return;
    mkdir(dst, 0755);
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char src_path[512], dst_path[512];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);
        DIR *sub = opendir(src_path);
        if (sub) { closedir(sub); copy_dir(src_path, dst_path); }
        else copy_file(src_path, dst_path);
    }
    closedir(d);
}