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
#include "../include/build.h"
#include "../include/serve.h"
#include "../include/init.h"
#include "../include/deploy.h"
TomlDoc g_toml;
char g_theme_path[512] = "themes/default";

void load_config(void) {
    if (toml_parse("config.toml", &g_toml) != 0) {
        fprintf(stderr, "Warning: config.toml not found, using defaults\n");
        return;
    }
    const char *theme = toml_get_or(&g_toml, "", "theme", "default");
    snprintf(g_theme_path, sizeof(g_theme_path), "themes/%s", theme);
}

void print_help(void) {
    printf("atomik-ssg - Lightweight static site generator\n\n");
    printf("Usage:\n");
    printf("  atomik-ssg init          Create a new project\n");
    printf("  atomik-ssg build         Generate site\n");
    printf("  atomik-ssg new <title>   Create a new post\n");
    printf("  atomik-ssg serve [port]  Start dev server (default: 4545)\n");
    printf("  atomik-ssg deploy        Build and deploy to VPS or another machine\n");
    printf("  atomik-ssg help          Show this message\n\n");
    printf("Example:\n");
    printf("  atomik-ssg new \"My First Post\"\n");
    printf("  atomik-ssg build\n");
    printf("  atomik-ssg serve\n");
}

void cmd_new(const char *title) {
    if (!title) {
        fprintf(stderr, "Usage: atomik-ssg new \"Post Title\"\n");
        return;
    }

    char slug[256] = {0};
    int j = 0;
    for (int i = 0; title[i] && j < 254; i++) {
        unsigned char c = (unsigned char)title[i];
        if (c >= 'A' && c <= 'Z') { slug[j++] = c + 32; continue; }
        if (c == ' ' || c == '_') { slug[j++] = '-'; continue; }
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')
            slug[j++] = c;
    }
    slug[j] = '\0';

    char *slug_start = slug;
    while (*slug_start == '-') slug_start++;
    int slen = strlen(slug_start);
    while (slen > 0 && slug_start[slen-1] == '-') slen--;
    slug_start[slen] = '\0';

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[32];
    strftime(date, sizeof(date), "%Y-%m-%d", tm);

    char path[512];
    snprintf(path, sizeof(path), "content/posts/%s-%s.md", date, slug_start);

    FILE *check = fopen(path, "r");
    if (check) { fclose(check); fprintf(stderr, "Error: %s already exists\n", path); return; }

    FILE *f = fopen(path, "w");
    if (!f) { perror(path); return; }
    fprintf(f,
        "---\n"
        "title: %s\n"
        "date: %s\n"
        "slug: %s\n"
        "description: \n"
        "---\n\n"
        "Write your content here...\n",
        title, date, slug_start);
    fclose(f);
    printf("Created: %s\n", path);
}

int main(int argc, char *argv[]) {
    load_config();

    if (argc < 2) { print_help(); return 0; }

    if (strcmp(argv[1], "init") == 0)
        cmd_init();
    else if (strcmp(argv[1], "build") == 0)
        cmd_build();
    else if (strcmp(argv[1], "new") == 0)
        cmd_new(argc > 2 ? argv[2] : NULL);
    else if (strcmp(argv[1], "serve") == 0) {
        int port = argc > 2 ? atoi(argv[2]) : 4545;
        cmd_serve(port);
    } else if (strcmp(argv[1], "help") == 0)
        print_help();
    else if (strcmp(argv[1], "new") == 0)
        cmd_new(argc > 2 ? argv[2] : NULL);
    else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        fprintf(stderr, "Run atomik-ssg help for usage\n");
        return 1;
    }

    return 0;
}