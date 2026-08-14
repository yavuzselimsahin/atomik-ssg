#include "../include/parser.h"
#include "../include/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <dirent.h>

#ifdef _WIN32
    #define strcasecmp _stricmp
#else
    #include <strings.h>
#endif

#define COPY_MAX_DEPTH 32

/* Accepts the spellings people actually write in frontmatter. */
static int is_truthy(const char *v) {
    return strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 ||
           strcasecmp(v, "on")   == 0 || strcmp(v, "1") == 0;
}

int parse_frontmatter(char *raw, Post *post) {
    if (!raw || strncmp(raw, "---", 3) != 0) return -1;
    char *ptr = raw + 3;
    while (*ptr == '\r' || *ptr == '\n') ptr++;

    char line[MAX_LINE];
    while (*ptr) {
        int i = 0;
        while (*ptr && *ptr != '\n' && i < MAX_LINE - 1)
            line[i++] = *ptr++;
        line[i] = '\0';
        while (*ptr && *ptr != '\n') ptr++;   /* drop an over-long line's tail */
        if (*ptr == '\n') ptr++;

        size_t llen = strlen(line);
        if (llen > 0 && line[llen - 1] == '\r') line[llen - 1] = '\0';

        if (strncmp(line, "---", 3) == 0) {
            while (*ptr == '\r' || *ptr == '\n') ptr++;
            post->content = ptr;
            return 0;
        }

        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = '\0';

        char *key = line;
        char *val = colon + 1;
        while (*val == ' ' || *val == '\t') val++;

        char *cr = strchr(val, '\r');
        if (cr) *cr = '\0';

        size_t vlen = strlen(val);
        while (vlen > 0 && (val[vlen - 1] == ' ' || val[vlen - 1] == '\t')) val[--vlen] = '\0';

        if      (strcmp(key, "title") == 0)       snprintf(post->title,       MAX_FIELD, "%s", val);
        else if (strcmp(key, "date") == 0)        snprintf(post->date,        MAX_FIELD, "%s", val);
        else if (strcmp(key, "slug") == 0)        snprintf(post->slug,        MAX_FIELD, "%s", val);
        else if (strcmp(key, "description") == 0) snprintf(post->description, MAX_FIELD, "%s", val);
        else if (strcmp(key, "draft") == 0)       post->draft = is_truthy(val);
        else if (strcmp(key, "order") == 0)       post->order = atoi(val);
    }
    return -1;   /* frontmatter never closed */
}

char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return NULL; }

    if (fseek(f, 0, SEEK_END) != 0) { perror(path); fclose(f); return NULL; }
    long size = ftell(f);
    if (size < 0) { perror(path); fclose(f); return NULL; }
    rewind(f);

    char *buf = malloc((size_t)size + 1);
    if (!buf) {
        fprintf(stderr, "Error: out of memory reading %s\n", path);
        fclose(f);
        return NULL;
    }

    size_t got = fread(buf, 1, (size_t)size, f);
    if (ferror(f)) {
        fprintf(stderr, "Error: read failed on %s\n", path);
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[got] = '\0';
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
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fprintf(stderr, "Warning: short write on %s\n", dst);
            break;
        }
    }
    fclose(in);
    fclose(out);
}

static void copy_dir_depth(const char *src, const char *dst, int depth) {
    if (depth > COPY_MAX_DEPTH) {
        fprintf(stderr, "Warning: directory nesting too deep, skipping %s\n", src);
        return;
    }

    DIR *d = opendir(src);
    if (!d) return;
    make_dir(dst);

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char src_path[1024], dst_path[1024];
        if (snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name) >= (int)sizeof(src_path) ||
            snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name) >= (int)sizeof(dst_path)) {
            fprintf(stderr, "Warning: path too long, skipping %s/%s\n", src, entry->d_name);
            continue;
        }

        DIR *sub = opendir(src_path);
        if (sub) { closedir(sub); copy_dir_depth(src_path, dst_path, depth + 1); }
        else       copy_file(src_path, dst_path);
    }
    closedir(d);
}

void copy_dir(const char *src, const char *dst) {
    copy_dir_depth(src, dst, 0);
}

/* ------------------------------------------------------------------ */

/* True when name starts with a "YYYY-MM-DD-" prefix. */
static int has_date_prefix(const char *name) {
    if (strlen(name) < 11) return 0;
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) { if (name[i] != '-') return 0; }
        else if (!isdigit((unsigned char)name[i])) return 0;
    }
    return name[10] == '-';
}

static void slug_from_filename(const char *fname, char *out, size_t out_size) {
    char base[512];
    snprintf(base, sizeof(base), "%s", fname);

    char *dot = strrchr(base, '.');
    if (dot) *dot = '\0';

    const char *p = has_date_prefix(base) ? base + 11 : base;
    slugify(p, out, out_size);
    if (out[0] == '\0') slugify(base, out, out_size);
}

static int post_cmp(const void *a, const void *b) {
    const Post *pa = (const Post *)a;
    const Post *pb = (const Post *)b;

    int c = strcmp(pb->date, pa->date);   /* ISO dates sort lexicographically */
    if (c != 0) return c;
    return strcmp(pa->slug, pb->slug);    /* stable tie-break */
}

static int postlist_push(PostList *list, const Post *p) {
    if (list->count == list->cap) {
        int   ncap = list->cap ? list->cap * 2 : 16;
        Post *np   = realloc(list->posts, (size_t)ncap * sizeof(Post));
        if (!np) return -1;
        list->posts = np;
        list->cap   = ncap;
    }
    list->posts[list->count++] = *p;
    return 0;
}

int collect_posts(const char *dirpath, PostList *list) {
    list->posts = NULL;
    list->count = 0;
    list->cap   = 0;

    DIR *d = opendir(dirpath);
    if (!d) return -1;

    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        const char *ext = strrchr(e->d_name, '.');
        if (!ext || strcmp(ext, ".md") != 0) continue;

        char path[1024];
        if (snprintf(path, sizeof(path), "%s/%s", dirpath, e->d_name) >= (int)sizeof(path)) {
            fprintf(stderr, "Warning: path too long, skipping %s\n", e->d_name);
            continue;
        }

        char *raw = read_file(path);
        if (!raw) continue;

        Post p;
        memset(&p, 0, sizeof(p));
        /* Unordered entries sort last, so writing `order: 1` promotes a page
           instead of demoting it below everything that says nothing. */
        p.order = INT_MAX;
        if (parse_frontmatter(raw, &p) != 0) {
            fprintf(stderr, "Warning: missing or unterminated frontmatter, skipping %s\n", path);
            free(raw);
            continue;
        }
        p.raw = raw;
        snprintf(p.source, sizeof(p.source), "%s", path);

        /* Always normalise the slug: it becomes a directory name, so it must
           not be able to contain '/', '..' or anything else path-significant. */
        if (p.slug[0] != '\0') {
            char norm[MAX_FIELD];
            slugify(p.slug, norm, sizeof(norm));
            snprintf(p.slug, sizeof(p.slug), "%s", norm);
        }
        if (p.slug[0] == '\0')
            slug_from_filename(e->d_name, p.slug, sizeof(p.slug));

        if (p.slug[0] == '\0') {
            fprintf(stderr, "Warning: cannot derive a slug, skipping %s\n", path);
            free(raw);
            continue;
        }

        if (p.date[0] == '\0' && has_date_prefix(e->d_name))
            snprintf(p.date, sizeof(p.date), "%.10s", e->d_name);

        if (p.title[0] == '\0')
            snprintf(p.title, sizeof(p.title), "%s", p.slug);

        if (postlist_push(list, &p) != 0) {
            fprintf(stderr, "Error: out of memory collecting posts\n");
            free(raw);
            break;
        }
    }
    closedir(d);

    if (list->count > 1)
        qsort(list->posts, (size_t)list->count, sizeof(Post), post_cmp);

    return 0;
}

/* ------------------------------------------------------------------ */

#define PAGE_MAX_DEPTH 16

/* Reads one markdown file into p, deriving whatever the frontmatter omitted.
   url_prefix is the already-slugified directory path it sits under. */
static int load_page(const char *fs_path, const char *fname,
                     const char *url_prefix, Post *p) {
    char *raw = read_file(fs_path);
    if (!raw) return -1;

    memset(p, 0, sizeof(*p));
    p->order = INT_MAX;
    if (parse_frontmatter(raw, p) != 0) {
        fprintf(stderr, "Warning: missing or unterminated frontmatter, skipping %s\n", fs_path);
        free(raw);
        return -1;
    }
    p->raw = raw;
    snprintf(p->source, sizeof(p->source), "%s", fs_path);

    /* The leaf name: an explicit slug replaces it, the file name supplies it
       otherwise. Either way it is reduced to one safe path segment. */
    char leaf[MAX_FIELD];
    if (p->slug[0] != '\0') {
        slugify(p->slug, leaf, sizeof(leaf));
    } else {
        char base[MAX_PATH];
        snprintf(base, sizeof(base), "%s", fname);
        char *dot = strrchr(base, '.');
        if (dot) *dot = '\0';
        slugify(base, leaf, sizeof(leaf));
    }

    /* index.md names the directory it lives in rather than a child of it. */
    int is_index = strcmp(leaf, "index") == 0;

    if (is_index) {
        snprintf(p->slug, sizeof(p->slug), "%s", url_prefix);
    } else if (url_prefix[0]) {
        snprintf(p->slug, sizeof(p->slug), "%s/%s", url_prefix, leaf);
    } else {
        snprintf(p->slug, sizeof(p->slug), "%s", leaf);
    }

    if (p->slug[0] == '\0') {
        fprintf(stderr, "Warning: %s would sit at the site root and overwrite the "
                        "generated index, skipping it\n", fs_path);
        free(raw);
        return -1;
    }

    if (p->title[0] == '\0') {
        const char *last = strrchr(p->slug, '/');
        snprintf(p->title, sizeof(p->title), "%s", last ? last + 1 : p->slug);
    }
    return 0;
}

static void walk_pages(const char *fs_dir, const char *url_prefix,
                       PostList *list, const char *skip_subdir, int depth) {
    if (depth > PAGE_MAX_DEPTH) {
        fprintf(stderr, "Warning: content nested deeper than %d levels, skipping %s\n",
                PAGE_MAX_DEPTH, fs_dir);
        return;
    }

    DIR *d = opendir(fs_dir);
    if (!d) return;

    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;

        char fs_path[MAX_PATH];
        if (snprintf(fs_path, sizeof(fs_path), "%s/%s", fs_dir, e->d_name) >= (int)sizeof(fs_path)) {
            fprintf(stderr, "Warning: path too long, skipping %s/%s\n", fs_dir, e->d_name);
            continue;
        }

        DIR *sub = opendir(fs_path);
        if (sub) {
            closedir(sub);
            /* content/posts/ is the dated half of the site, not part of the tree. */
            if (depth == 0 && skip_subdir && strcmp(e->d_name, skip_subdir) == 0) continue;

            char seg[MAX_FIELD], next_prefix[MAX_PATH];
            slugify(e->d_name, seg, sizeof(seg));
            if (seg[0] == '\0') continue;

            if (url_prefix[0])
                snprintf(next_prefix, sizeof(next_prefix), "%s/%s", url_prefix, seg);
            else
                snprintf(next_prefix, sizeof(next_prefix), "%s", seg);

            walk_pages(fs_path, next_prefix, list, skip_subdir, depth + 1);
            continue;
        }

        const char *ext = strrchr(e->d_name, '.');
        if (!ext || strcmp(ext, ".md") != 0) continue;

        Post p;
        if (load_page(fs_path, e->d_name, url_prefix, &p) != 0) continue;
        if (postlist_push(list, &p) != 0) {
            fprintf(stderr, "Error: out of memory collecting pages\n");
            free(p.raw);
            break;
        }
    }
    closedir(d);
}

int collect_pages(const char *dir, PostList *list, const char *skip_subdir) {
    list->posts = NULL;
    list->count = 0;
    list->cap   = 0;

    DIR *probe = opendir(dir);
    if (!probe) return -1;
    closedir(probe);

    walk_pages(dir, "", list, skip_subdir, 0);
    return 0;
}

void postlist_free(PostList *list) {
    for (int i = 0; i < list->count; i++)
        free(list->posts[i].raw);
    free(list->posts);
    list->posts = NULL;
    list->count = 0;
    list->cap   = 0;
}
