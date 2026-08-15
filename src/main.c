#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "../toml.h"
#include "../include/parser.h"
#include "../include/util.h"
#include "../include/globals.h"
#include "../include/build.h"
#include "../include/serve.h"
#include "../include/init.h"
#include "../include/deploy.h"

TomlDoc g_toml;
char    g_theme_path[512]              = "themes/default";
char    g_output_dir[256]              = "public";
char    g_site_title[MAX_FIELD]        = "Atomik SSG";
char    g_site_description[MAX_FIELD]  = "";
int     g_include_drafts               = 0;
const char *g_nav_html                 = "";
const char *g_tree_html                = "";
char    g_edit_url[512]                = "";
char    g_built_with[256]              = "";
char    g_base_path[256]               = "";
char    g_version[64]                  = "";

#define DEFAULT_PORT 4545

/* An output directory may be nested ("docs/manual"), but it still has to stay
   inside the project: no absolute paths, no climbing out. */
static int is_safe_relpath(const char *s) {
    if (!s || !*s) return 0;
    if (*s == '/' || strchr(s, '\\')) return 0;

    char buf[256];
    if (snprintf(buf, sizeof(buf), "%s", s) >= (int)sizeof(buf)) return 0;

    for (char *tok = strtok(buf, "/"); tok; tok = strtok(NULL, "/"))
        if (strcmp(tok, "..") == 0 || strcmp(tok, ".") == 0) return 0;
    return 1;
}

/* A config value that becomes part of a path must not contain separators. */
static int is_plain_name(const char *s) {
    if (!s || !*s) return 0;
    if (strcmp(s, ".") == 0 || strcmp(s, "..") == 0) return 0;
    for (; *s; s++)
        if (*s == '/' || *s == '\\') return 0;
    return 1;
}

static void load_config(int quiet) {
    if (toml_parse("config.toml", &g_toml) != 0) {
        if (!quiet)
            fprintf(stderr, "Warning: config.toml not found, using defaults\n");
        return;
    }

    const char *theme = toml_get_or(&g_toml, "", "theme", "default");
    if (!is_plain_name(theme)) {
        fprintf(stderr, "Warning: invalid theme name \"%s\", using default\n", theme);
        theme = "default";
    }
    snprintf(g_theme_path, sizeof(g_theme_path), "themes/%s", theme);

    const char *out = toml_get_or(&g_toml, "build", "output_dir", "public");
    if (!is_safe_relpath(out)) {
        fprintf(stderr, "Warning: invalid output_dir \"%s\", using public\n", out);
        out = "public";
    }
    snprintf(g_output_dir, sizeof(g_output_dir), "%s", out);
    size_t olen = strlen(g_output_dir);
    while (olen > 1 && g_output_dir[olen - 1] == '/') g_output_dir[--olen] = '\0';

    snprintf(g_site_title, sizeof(g_site_title), "%s",
             toml_get_or(&g_toml, "", "title", "Atomik SSG"));
    snprintf(g_site_description, sizeof(g_site_description), "%s",
             toml_get_or(&g_toml, "", "description", ""));

    /* Trailing slash trimmed so "{{edit_url}}" never comes out doubled. */
    snprintf(g_edit_url, sizeof(g_edit_url), "%s",
             toml_get_or(&g_toml, "", "edit_url", ""));
    size_t elen = strlen(g_edit_url);
    while (elen > 0 && g_edit_url[elen - 1] == '/') g_edit_url[--elen] = '\0';

    snprintf(g_version, sizeof(g_version), "%s",
             toml_get_or(&g_toml, "", "version", ""));

    /* Normalised to "" or "/prefix": a leading slash, never a trailing one,
       so every call site can join it without thinking about separators. */
    const char *bp = toml_get_or(&g_toml, "", "base_path", "");
    if (strstr(bp, "..") || strchr(bp, ' ')) {
        fprintf(stderr, "Warning: invalid base_path \"%s\", ignoring it\n", bp);
        bp = "";
    }
    while (*bp == '/') bp++;
    if (*bp) {
        snprintf(g_base_path, sizeof(g_base_path), "/%s", bp);
        size_t blen = strlen(g_base_path);
        while (blen > 1 && g_base_path[blen - 1] == '/') g_base_path[--blen] = '\0';
    }

    /* Absent means off, so commenting the line out is enough to remove it. */
    if (is_truthy(toml_get(&g_toml, "", "built_with")))
        snprintf(g_built_with, sizeof(g_built_with),
                 "<a href=\"https://github.com/yavuzselimsahin/atomik-ssg\">"
                 "Generated with atomik-ssg</a>");
}

/* Removes flag from argv if present, so the positional arguments stay simple. */
static int take_flag(int *argc, char *argv[], const char *flag) {
    for (int i = 1; i < *argc; i++) {
        if (strcmp(argv[i], flag) == 0) {
            for (int j = i; j < *argc - 1; j++) argv[j] = argv[j + 1];
            (*argc)--;
            return 1;
        }
    }
    return 0;
}

/* Returns the port, or -1 if the text is not a usable port number. */
static int parse_port(const char *s) {
    char *end;
    long  v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || v < 1 || v > 65535) return -1;
    return (int)v;
}

static int configured_port(void) {
    const char *s = toml_get(&g_toml, "server", "port");
    if (!s) return DEFAULT_PORT;

    int p = parse_port(s);
    if (p < 0) {
        fprintf(stderr, "Warning: invalid [server] port \"%s\", using %d\n", s, DEFAULT_PORT);
        return DEFAULT_PORT;
    }
    return p;
}

void print_help(void) {
    printf("atomik-ssg - Lightweight static site generator\n\n");
    printf("Usage:\n");
    printf("  atomik-ssg init            Create a new project\n");
    printf("  atomik-ssg build [--drafts]  Generate site\n");
    printf("  atomik-ssg new <title>     Create a new post\n");
    printf("  atomik-ssg serve [port]    Start dev server (default: %d)\n", DEFAULT_PORT);
    printf("  atomik-ssg deploy [--drafts] Build and deploy to VPS or another machine\n");
    printf("  atomik-ssg help            Show this message\n\n");
    printf("Content:\n");
    printf("  content/posts/*.md   dated posts, listed on the index and in the feed\n");
    printf("  content/*.md         standalone pages, published at /<slug>/\n");
    printf("  draft: true          keeps an entry out of the build\n\n");
    printf("Example:\n");
    printf("  atomik-ssg new My First Post\n");
    printf("  atomik-ssg build\n");
    printf("  atomik-ssg serve\n");
}

/* Joins the remaining arguments so `new My First Post` works without quotes. */
static char *join_args(int argc, char *argv[], int from) {
    StrBuf sb;
    sb_init(&sb);

    for (int i = from; i < argc; i++) {
        if (i > from && sb_append(&sb, " ") != 0) { sb_free(&sb); return NULL; }
        if (sb_append(&sb, argv[i]) != 0)         { sb_free(&sb); return NULL; }
    }
    return sb.data;
}

int cmd_new(const char *title) {
    if (!title || !*title) {
        fprintf(stderr, "Usage: atomik-ssg new <title>\n");
        return 1;
    }

    char slug[MAX_FIELD];
    slugify(title, slug, sizeof(slug));
    if (slug[0] == '\0') {
        fprintf(stderr, "Error: could not build a slug from \"%s\"\n", title);
        return 1;
    }

    time_t     t  = time(NULL);
    struct tm *tm = localtime(&t);
    char       date[32];
    if (!tm || strftime(date, sizeof(date), "%Y-%m-%d", tm) == 0) {
        fprintf(stderr, "Error: could not determine today's date\n");
        return 1;
    }

    char path[512];
    if (snprintf(path, sizeof(path), "content/posts/%s-%s.md", date, slug) >= (int)sizeof(path)) {
        fprintf(stderr, "Error: title too long\n");
        return 1;
    }

    FILE *check = fopen(path, "r");
    if (check) {
        fclose(check);
        fprintf(stderr, "Error: %s already exists\n", path);
        return 1;
    }

    FILE *f = fopen(path, "w");
    if (!f) { perror(path); return 1; }

    fprintf(f,
        "---\n"
        "title: %s\n"
        "date: %s\n"
        "slug: %s\n"
        "description: \n"
        "draft: false\n"
        "---\n\n"
        "Write your content here...\n",
        title, date, slug);

    if (fclose(f) != 0) { fprintf(stderr, "Error: could not write %s\n", path); return 1; }

    printf("Created: %s\n", path);
    return 0;
}

int main(int argc, char *argv[]) {
    int wants_drafts = take_flag(&argc, argv, "--drafts");

    if (argc < 2) {
        load_config(1);
        print_help();
        return 0;
    }

    const char *cmd   = argv[1];
    int         quiet = strcmp(cmd, "init") == 0 || strcmp(cmd, "help") == 0;

    int builds = strcmp(cmd, "build") == 0 || strcmp(cmd, "deploy") == 0;
    if (wants_drafts && !builds) {
        fprintf(stderr, "Error: --drafts only applies to build and deploy\n");
        return 1;
    }
    g_include_drafts = wants_drafts;

    load_config(quiet);

    if (strcmp(cmd, "init") == 0) {
        cmd_init();
    } else if (strcmp(cmd, "build") == 0) {
        cmd_build();
    } else if (strcmp(cmd, "new") == 0) {
        if (argc < 3) { fprintf(stderr, "Usage: atomik-ssg new <title>\n"); return 1; }
        char *title = join_args(argc, argv, 2);
        if (!title) { fprintf(stderr, "Error: out of memory\n"); return 1; }
        int rc = cmd_new(title);
        free(title);
        return rc;
    } else if (strcmp(cmd, "serve") == 0) {
        int port = configured_port();
        if (argc > 2) {
            port = parse_port(argv[2]);
            if (port < 0) {
                fprintf(stderr, "Error: invalid port \"%s\" (expected 1-65535)\n", argv[2]);
                return 1;
            }
        }
        cmd_serve(port);
    } else if (strcmp(cmd, "help") == 0 ||
               strcmp(cmd, "--help") == 0 ||
               strcmp(cmd, "-h") == 0) {
        print_help();
    } else if (strcmp(cmd, "deploy") == 0) {
        return cmd_deploy();
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        fprintf(stderr, "Run atomik-ssg help for usage\n");
        return 1;
    }

    return 0;
}
