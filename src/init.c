#include "../include/init.h"
#include "../include/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <sys/stat.h>
#endif

#define MKPATH(sub) \
    snprintf(path, sizeof(path), "%s/%s", name, sub); \
    mkdir(path, 0755);

static void prompt(const char *question, const char *fallback,
                   char *out, int size) {
    printf("%s", question);
    if (fallback && fallback[0]) printf(" [%s]", fallback);
    printf(": ");
    fflush(stdout);

    if (!fgets(out, size, stdin)) { out[0] = '\0'; return; }

    char *nl = strchr(out, '\n');
    if (nl) *nl = '\0';
    char *cr = strchr(out, '\r');
    if (cr) *cr = '\0';

    if (out[0] == '\0' && fallback)
        strncpy(out, fallback, size - 1);
}

void cmd_init(void) {
    char name[256]   = {0};
    char title[256]  = {0};
    char desc[256]   = {0};
    char author[256] = {0};
    char url[256]    = {0};
    char theme_choice[32] = {0};
    char theme_name[32]   = {0};

    printf("\nWelcome to atomik-ssg!\n");
    printf("-----------------------------\n\n");

    prompt("Project name",   "my-blog",         name,   sizeof(name));
    prompt("Site title",     "My Blog",          title,  sizeof(title));
    prompt("Description",    "",                 desc,   sizeof(desc));
    prompt("Author",         "",                 author, sizeof(author));
    prompt("Base URL",       "http://localhost", url,    sizeof(url));

    printf("\nAvailable themes:\n");
    printf("  1) default  (light, minimal)\n");
    printf("  2) dark     (terminal feel)\n");
    printf("  3) sepia    (warm, book-like)\n\n");
    prompt("Theme", "1", theme_choice, sizeof(theme_choice));

    if (strcmp(theme_choice, "2") == 0)
        strncpy(theme_name, "dark", sizeof(theme_name) - 1);
    else if (strcmp(theme_choice, "3") == 0)
        strncpy(theme_name, "sepia", sizeof(theme_name) - 1);
    else
        strncpy(theme_name, "default", sizeof(theme_name) - 1);

    printf("\n");

    if (mkdir(name, 0755) != 0) {
        fprintf(stderr, "Error: directory '%s' already exists\n", name);
        return;
    }

    char path[512];
    MKPATH("themes");
    MKPATH("themes/default");
    MKPATH("themes/default/templates");
    MKPATH("themes/default/static");
    MKPATH("themes/dark");
    MKPATH("themes/dark/templates");
    MKPATH("themes/dark/static");
    MKPATH("themes/sepia");
    MKPATH("themes/sepia/templates");
    MKPATH("themes/sepia/static");
    MKPATH("content");
    MKPATH("content/posts");
    MKPATH("static");
    MKPATH("static/images");
    MKPATH("public");

    /* config.toml */
    snprintf(path, sizeof(path), "%s/config.toml", name);
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f,
            "title       = \"%s\"\n"
            "description = \"%s\"\n"
            "base_url    = \"%s\"\n"
            "author      = \"%s\"\n"
            "theme       = \"%s\"\n\n"
            "[build]\n"
            "output_dir = \"public\"\n\n"
            "[server]\n"
            "port = 4545\n",
            title, desc, url, author, theme_name);
        fclose(f);
        printf("  created  %s/config.toml\n", name);
    }

    /* Templates — her tema için */
    const char *themes[] = {"default", "dark", "sepia"};
    for (int t = 0; t < 3; t++) {
        snprintf(path, sizeof(path), "%s/themes/%s/templates/index.html", name, themes[t]);
        f = fopen(path, "w");
        if (f) {
            fprintf(f,
                "<!DOCTYPE html>\n"
                "<html lang=\"en\">\n"
                "<head>\n"
                "    <meta charset=\"UTF-8\">\n"
                "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                "    <title>{{title}}</title>\n"
                "    <meta name=\"description\" content=\"{{description}}\">\n"
                "    <link rel=\"stylesheet\" href=\"/style.css\">\n"
                "</head>\n"
                "<body>\n"
                "    <header>\n"
                "        <h1>{{title}}</h1>\n"
                "        <p>{{description}}</p>\n"
                "    </header>\n"
                "    <main>\n"
                "        <ul class=\"post-list\">\n"
                "{{post_items}}\n"
                "        </ul>\n"
                "    </main>\n"
                "</body>\n"
                "</html>\n");
            fclose(f);
            printf("  created  %s/themes/%s/templates/index.html\n", name, themes[t]);
        }

        snprintf(path, sizeof(path), "%s/themes/%s/templates/post.html", name, themes[t]);
        f = fopen(path, "w");
        if (f) {
            fprintf(f,
                "<!DOCTYPE html>\n"
                "<html lang=\"en\">\n"
                "<head>\n"
                "    <meta charset=\"UTF-8\">\n"
                "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                "    <title>{{title}}</title>\n"
                "    <meta name=\"description\" content=\"{{description}}\">\n"
                "    <link rel=\"stylesheet\" href=\"/style.css\">\n"
                "</head>\n"
                "<body>\n"
                "    <header>\n"
                "        <a href=\"/\">&larr; Home</a>\n"
                "    </header>\n"
                "    <main>\n"
                "        <article>\n"
                "            <h1>{{title}}</h1>\n"
                "            <time>{{date}}</time>\n"
                "            {{content}}\n"
                "        </article>\n"
                "    </main>\n"
                "</body>\n"
                "</html>\n");
            fclose(f);
            printf("  created  %s/themes/%s/templates/post.html\n", name, themes[t]);
        }
    }

    /* CSS — default */
    snprintf(path, sizeof(path), "%s/themes/default/static/style.css", name);
    f = fopen(path, "w");
    if (f) {
        fprintf(f,
            ":root {\n"
            "    --bg:      #ffffff;\n"
            "    --border:  #e2e8f0;\n"
            "    --text:    #1a202c;\n"
            "    --muted:   #718096;\n"
            "    --accent:  #2b6cb0;\n"
            "    --code-bg: #f7fafc;\n"
            "}\n"
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n"
            "body { background: var(--bg); color: var(--text);\n"
            "    font-family: Georgia, serif; font-size: 1.05rem;\n"
            "    line-height: 1.8; max-width: 680px;\n"
            "    margin: 0 auto; padding: 2rem 1.5rem; }\n"
            "header { border-bottom: 1px solid var(--border);\n"
            "    padding-bottom: 1.5rem; margin-bottom: 2.5rem; }\n"
            "header a { color: var(--accent); text-decoration: none;\n"
            "    font-size: 0.9rem; font-family: monospace; }\n"
            "header h1 { font-size: 1.4rem; margin-bottom: 0.3rem; }\n"
            "header p { color: var(--muted); font-size: 0.9rem; }\n"
            ".post-list { list-style: none; }\n"
            ".post-list li { padding: 1.2rem 0; border-bottom: 1px solid var(--border); }\n"
            ".post-list time { font-family: monospace; font-size: 0.85rem;\n"
            "    color: var(--muted); display: block; margin-bottom: 0.3rem; }\n"
            ".post-list a { color: var(--text); text-decoration: none;\n"
            "    font-size: 1.1rem; font-weight: bold; }\n"
            ".post-list a:hover { color: var(--accent); }\n"
            ".post-list p { color: var(--muted); font-size: 0.9rem; margin-top: 0.3rem; }\n"
            "article h1 { font-size: 1.8rem; margin-bottom: 0.5rem; }\n"
            "article time { font-family: monospace; font-size: 0.85rem;\n"
            "    color: var(--muted); display: block; margin-bottom: 2rem;\n"
            "    padding-bottom: 1rem; border-bottom: 1px solid var(--border); }\n"
            "article h2 { font-size: 1.3rem; margin: 2rem 0 0.8rem; }\n"
            "article h3 { font-size: 1.1rem; margin: 1.5rem 0 0.6rem; }\n"
            "article p { margin-bottom: 1.2rem; }\n"
            "article a { color: var(--accent); }\n"
            "pre { background: var(--code-bg); border: 1px solid var(--border);\n"
            "    border-radius: 4px; padding: 1.2rem; overflow-x: auto;\n"
            "    margin: 1.5rem 0; font-size: 0.88rem; }\n"
            "code { font-family: monospace; font-size: 0.88em; }\n"
            "p code { background: var(--code-bg); border: 1px solid var(--border);\n"
            "    padding: 0.15em 0.4em; border-radius: 3px; }\n"
            "blockquote { border-left: 3px solid var(--border);\n"
            "    padding-left: 1rem; color: var(--muted); margin: 1.5rem 0; }\n");
        fclose(f);
        printf("  created  %s/themes/default/static/style.css\n", name);
    }

    /* CSS — dark */
    snprintf(path, sizeof(path), "%s/themes/dark/static/style.css", name);
    f = fopen(path, "w");
    if (f) {
        fprintf(f,
            ":root {\n"
            "    --bg:      #0f1117;\n"
            "    --border:  #2a2d3a;\n"
            "    --text:    #e2e8f0;\n"
            "    --muted:   #64748b;\n"
            "    --accent:  #60a5fa;\n"
            "    --code-bg: #141720;\n"
            "}\n"
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n"
            "body { background: var(--bg); color: var(--text);\n"
            "    font-family: monospace; font-size: 1rem;\n"
            "    line-height: 1.8; max-width: 680px;\n"
            "    margin: 0 auto; padding: 2rem 1.5rem; }\n"
            "header { border-bottom: 1px solid var(--border);\n"
            "    padding-bottom: 1.5rem; margin-bottom: 2.5rem; }\n"
            "header a { color: var(--accent); text-decoration: none; font-size: 0.9rem; }\n"
            "header h1 { font-size: 1.4rem; margin-bottom: 0.3rem; color: var(--accent); }\n"
            "header p { color: var(--muted); font-size: 0.9rem; }\n"
            ".post-list { list-style: none; }\n"
            ".post-list li { padding: 1.2rem 0; border-bottom: 1px solid var(--border); }\n"
            ".post-list time { font-size: 0.85rem; color: var(--muted);\n"
            "    display: block; margin-bottom: 0.3rem; }\n"
            ".post-list a { color: var(--text); text-decoration: none;\n"
            "    font-size: 1.1rem; font-weight: bold; }\n"
            ".post-list a:hover { color: var(--accent); }\n"
            ".post-list p { color: var(--muted); font-size: 0.9rem; margin-top: 0.3rem; }\n"
            "article h1 { font-size: 1.8rem; margin-bottom: 0.5rem; color: var(--accent); }\n"
            "article time { font-size: 0.85rem; color: var(--muted); display: block;\n"
            "    margin-bottom: 2rem; padding-bottom: 1rem;\n"
            "    border-bottom: 1px solid var(--border); }\n"
            "article h2 { font-size: 1.3rem; margin: 2rem 0 0.8rem; color: var(--accent); }\n"
            "article h3 { font-size: 1.1rem; margin: 1.5rem 0 0.6rem; }\n"
            "article p { margin-bottom: 1.2rem; }\n"
            "article a { color: var(--accent); }\n"
            "pre { background: var(--code-bg); border: 1px solid var(--border);\n"
            "    border-radius: 4px; padding: 1.2rem; overflow-x: auto;\n"
            "    margin: 1.5rem 0; font-size: 0.88rem; }\n"
            "code { font-family: monospace; font-size: 0.88em; }\n"
            "p code { background: var(--code-bg); border: 1px solid var(--border);\n"
            "    padding: 0.15em 0.4em; border-radius: 3px; }\n"
            "blockquote { border-left: 3px solid var(--accent);\n"
            "    padding-left: 1rem; color: var(--muted); margin: 1.5rem 0; }\n");
        fclose(f);
        printf("  created  %s/themes/dark/static/style.css\n", name);
    }

    /* CSS — sepia */
    snprintf(path, sizeof(path), "%s/themes/sepia/static/style.css", name);
    f = fopen(path, "w");
    if (f) {
        fprintf(f,
            ":root {\n"
            "    --bg:      #f8f1e4;\n"
            "    --border:  #d4c5a9;\n"
            "    --text:    #3d2b1f;\n"
            "    --muted:   #8c7560;\n"
            "    --accent:  #8b4513;\n"
            "    --code-bg: #ede8dc;\n"
            "}\n"
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n"
            "body { background: var(--bg); color: var(--text);\n"
            "    font-family: 'Palatino Linotype', Palatino, Georgia, serif;\n"
            "    font-size: 1.1rem; line-height: 1.9; max-width: 680px;\n"
            "    margin: 0 auto; padding: 2rem 1.5rem; }\n"
            "header { border-bottom: 2px solid var(--border);\n"
            "    padding-bottom: 1.5rem; margin-bottom: 2.5rem; }\n"
            "header a { color: var(--accent); text-decoration: none; font-size: 0.9rem; }\n"
            "header h1 { font-size: 1.6rem; margin-bottom: 0.3rem; font-style: italic; }\n"
            "header p { color: var(--muted); font-size: 0.9rem; }\n"
            ".post-list { list-style: none; }\n"
            ".post-list li { padding: 1.2rem 0; border-bottom: 1px solid var(--border); }\n"
            ".post-list time { font-size: 0.85rem; color: var(--muted);\n"
            "    display: block; margin-bottom: 0.3rem; font-style: italic; }\n"
            ".post-list a { color: var(--text); text-decoration: none;\n"
            "    font-size: 1.1rem; font-weight: bold; }\n"
            ".post-list a:hover { color: var(--accent); }\n"
            ".post-list p { color: var(--muted); font-size: 0.9rem; margin-top: 0.3rem; }\n"
            "article h1 { font-size: 2rem; margin-bottom: 0.5rem; font-style: italic; }\n"
            "article time { font-size: 0.85rem; color: var(--muted); display: block;\n"
            "    margin-bottom: 2rem; padding-bottom: 1rem;\n"
            "    border-bottom: 1px solid var(--border); font-style: italic; }\n"
            "article h2 { font-size: 1.4rem; margin: 2rem 0 0.8rem; }\n"
            "article h3 { font-size: 1.1rem; margin: 1.5rem 0 0.6rem; }\n"
            "article p { margin-bottom: 1.2rem; text-align: justify; }\n"
            "article a { color: var(--accent); }\n"
            "pre { background: var(--code-bg); border: 1px solid var(--border);\n"
            "    border-radius: 4px; padding: 1.2rem; overflow-x: auto;\n"
            "    margin: 1.5rem 0; font-size: 0.88rem; }\n"
            "code { font-family: monospace; font-size: 0.88em; }\n"
            "p code { background: var(--code-bg); border: 1px solid var(--border);\n"
            "    padding: 0.15em 0.4em; border-radius: 3px; }\n"
            "blockquote { border-left: 3px solid var(--accent);\n"
            "    padding-left: 1rem; color: var(--muted); margin: 1.5rem 0; font-style: italic; }\n");
        fclose(f);
        printf("  created  %s/themes/sepia/static/style.css\n", name);
    }

    /* Example post */
    snprintf(path, sizeof(path), "%s/content/posts/2026-08-10-hello-world.md", name);
    f = fopen(path, "w");
    if (f) {
        fprintf(f,
            "---\n"
            "title: Hello World\n"
            "date: 2026-08-10\n"
            "slug: hello-world\n"
            "description: My first post\n"
            "---\n\n"
            "Welcome to my blog. This is the first post.\n");
        fclose(f);
        printf("  created  %s/content/posts/2026-08-10-hello-world.md\n", name);
    }

    printf("\nDone! Next steps:\n\n");
    printf("  cd %s\n", name);
    printf("  atomik-ssg build\n");
    printf("  atomik-ssg serve\n\n");
}