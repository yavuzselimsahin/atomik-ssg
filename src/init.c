#include "../include/init.h"
#include "../include/parser.h"
#include "../include/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>

#ifdef _WIN32
    #define strcasecmp _stricmp
#else
    #include <strings.h>
#endif

#define MKPATH(sub)                                            \
    do {                                                       \
        snprintf(path, sizeof(path), "%s/%s", name, sub);      \
        if (make_dir(path) != 0) { perror(path); return; }     \
    } while (0)

/* Post navigation, shared by all three themes. The :empty rule is what hides
   the link on the first and last post — the template engine has no
   conditionals, and for this it does not need any. */
#define NAV_CSS \
    ".site-nav ul { list-style: none; display: flex; flex-wrap: wrap;\n" \
    "    gap: 1rem; margin-top: 0.8rem; padding: 0; }\n" \
    ".site-nav ul:empty { display: none; }\n" \
    ".site-nav a { color: var(--muted); text-decoration: none;\n" \
    "    font-size: 0.85rem; }\n" \
    ".site-nav a:hover { color: var(--accent); }\n" \
    ".post-nav { display: flex; justify-content: space-between; gap: 1rem;\n" \
    "    margin-top: 3rem; }\n" \
    ".post-nav a { color: var(--accent); text-decoration: none;\n" \
    "    font-size: 0.9rem; max-width: 45%%; }\n" \
    ".post-nav a:empty { display: none; }\n" \
    ".post-nav .prev::before { content: \"\\2190  \"; }\n" \
    ".post-nav .next { margin-left: auto; text-align: right; }\n" \
    ".post-nav .next::after { content: \"  \\2192\"; }\n"

/* Starter pages offered by init. The order the user picks them in becomes the
   `order:` field, so the menu comes out in the order they asked for. */
static const struct {
    const char *slug;
    const char *title;
    const char *summary;      /* shown in the picker */
    const char *description;  /* frontmatter description */
    const char *body;
} PAGE_TEMPLATES[] = {
    { "about", "About", "Who you are", "About me and this site",
      "Write a few sentences about yourself here: what you work on, what this\n"
      "site is for, and why someone might want to read it.\n" },

    { "projects", "Projects", "What you have built", "Things I have built",
      "## Project name\n\n"
      "One sentence on what it does and why it exists.\n"
      "[Source](https://github.com/you/project)\n\n"
      "## Another project\n\n"
      "Replace these with your own.\n" },

    { "contact", "Contact", "How to reach you", "How to get in touch",
      "- Email: <you@example.com>\n"
      "- GitHub: <https://github.com/you>\n\n"
      "Replace these with the ways you actually want to be reached.\n" },

    { "uses", "Uses", "Your tools and setup", "The tools I use",
      "## Editor\n\n## Terminal\n\n## Hardware\n\n"
      "List what you actually reach for every day.\n" },

    { "now", "Now", "What you are working on", "What I am doing at the moment",
      "What you are focused on right now. The convention is to write it as if\n"
      "answering a friend you have not seen in a year, and to update it when\n"
      "that answer changes.\n" },
};

#define PAGE_COUNT ((int)(sizeof(PAGE_TEMPLATES) / sizeof(PAGE_TEMPLATES[0])))

static void prompt(const char *question, const char *fallback,
                   char *out, int size) {
    printf("%s", question);
    if (fallback && fallback[0]) printf(" [%s]", fallback);
    printf(": ");
    fflush(stdout);

    /* On EOF — a piped, non-interactive init — fall back rather than leaving
       the answer empty, so the same defaults apply as when a user hits Enter. */
    if (!fgets(out, size, stdin)) {
        snprintf(out, (size_t)size, "%s", fallback ? fallback : "");
        printf("\n");
        return;
    }

    char *nl = strchr(out, '\n');
    if (nl) *nl = '\0';
    char *cr = strchr(out, '\r');
    if (cr) *cr = '\0';

    if (out[0] == '\0' && fallback)
        snprintf(out, (size_t)size, "%s", fallback);
}

/* Accepts either the number or the name of a starter page. */
static int page_index(const char *token) {
    char *end;
    long  n = strtol(token, &end, 10);
    if (end != token && *end == '\0' && n >= 1 && n <= PAGE_COUNT)
        return (int)(n - 1);

    for (int i = 0; i < PAGE_COUNT; i++)
        if (strcasecmp(token, PAGE_TEMPLATES[i].slug) == 0) return i;

    return -1;
}

/* Fills picked[] with template indices in the order the user listed them,
   ignoring duplicates. Returns how many were understood. */
static int parse_page_choice(const char *input, int *picked) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", input);

    int count = 0;
    for (char *tok = strtok(buf, " ,;"); tok && count < PAGE_COUNT;
         tok = strtok(NULL, " ,;")) {

        int idx = page_index(tok);
        if (idx < 0) {
            fprintf(stderr, "  (ignoring unknown page \"%s\")\n", tok);
            continue;
        }
        int seen = 0;
        for (int i = 0; i < count; i++)
            if (picked[i] == idx) seen = 1;
        if (!seen) picked[count++] = idx;
    }
    return count;
}

static void write_page(const char *project, int idx, int order,
                       const char *author, const char *site_desc) {
    char path[512];
    snprintf(path, sizeof(path), "%s/content/%s.md",
             project, PAGE_TEMPLATES[idx].slug);

    FILE *f = fopen(path, "w");
    if (!f) { perror(path); return; }

    fprintf(f,
        "---\n"
        "title: %s\n"
        "slug: %s\n"
        "description: %s\n"
        "order: %d\n"
        "---\n\n",
        PAGE_TEMPLATES[idx].title, PAGE_TEMPLATES[idx].slug,
        PAGE_TEMPLATES[idx].description, order);

    /* The About page is the one init can genuinely prefill, since it already
       asked for the author and the site description. */
    if (strcmp(PAGE_TEMPLATES[idx].slug, "about") == 0) {
        if (author && author[0]) fprintf(f, "Hi, I am %s.\n\n", author);
        if (site_desc && site_desc[0]) fprintf(f, "%s\n\n", site_desc);
    }

    fputs(PAGE_TEMPLATES[idx].body, f);
    fclose(f);
    printf("  created  %s/content/%s.md\n", project, PAGE_TEMPLATES[idx].slug);
}

void cmd_init(void) {
    char name[256]   = {0};
    char title[256]  = {0};
    char desc[256]   = {0};
    char author[256] = {0};
    char url[256]    = {0};
    char theme_choice[32] = {0};
    char theme_name[32]   = {0};
    char page_choice[256] = {0};
    char deploy_host[256] = {0};
    char deploy_path[256] = {0};

    printf("\nWelcome to atomik-ssg!\n");
    printf("-----------------------------\n\n");

    prompt("Project name",   "my-blog",         name,   sizeof(name));
    prompt("Site title",     "My Blog",          title,  sizeof(title));
    prompt("Description",    "",                 desc,   sizeof(desc));
    prompt("Author",         "",                 author, sizeof(author));
    prompt("Base URL",       "http://localhost", url,    sizeof(url));

    printf("\nDeploy settings (optional, press Enter to skip):\n");
    prompt("VPS host", "user@vps", deploy_host, sizeof(deploy_host));
    prompt("Deploy path", "/var/www/myblog", deploy_path, sizeof(deploy_path));

    printf("\nAvailable themes:\n");
    printf("  1) default  (light, minimal)\n");
    printf("  2) dark     (terminal feel)\n");
    printf("  3) sepia    (warm, book-like)\n\n");
    prompt("Theme", "1", theme_choice, sizeof(theme_choice));

    printf("\nStarter pages (these become the site menu, in the order you list them):\n");
    for (int i = 0; i < PAGE_COUNT; i++)
        printf("  %d) %-9s %s\n", i + 1, PAGE_TEMPLATES[i].slug, PAGE_TEMPLATES[i].summary);
    printf("\n");
    prompt("Pages, comma separated", "about", page_choice, sizeof(page_choice));

    if (strcmp(theme_choice, "2") == 0)
        strncpy(theme_name, "dark", sizeof(theme_name) - 1);
    else if (strcmp(theme_choice, "3") == 0)
        strncpy(theme_name, "sepia", sizeof(theme_name) - 1);
    else
        strncpy(theme_name, "default", sizeof(theme_name) - 1);

    printf("\n");

    if (name[0] == '\0' || strchr(name, '/') || strchr(name, '\\') ||
        strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        fprintf(stderr, "Error: '%s' is not a valid project name\n", name);
        return;
    }

    /* make_dir() tolerates an existing directory; init must not reuse one. */
    DIR *existing = opendir(name);
    if (existing) {
        closedir(existing);
        fprintf(stderr, "Error: directory '%s' already exists\n", name);
        return;
    }
    if (make_dir(name) != 0) { perror(name); return; }

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
            "[deploy]\n"
            "host = \"%s\"\n"
            "path = \"%s\"\n\n"
            "[build]\n"
            "output_dir = \"public\"\n\n"
            "[server]\n"
            "port = 4545\n",
            title, desc, url, author, theme_name, deploy_host, deploy_path);
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
                "        <nav class=\"site-nav\"><ul>{{pages}}</ul></nav>\n"
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
                "        <nav class=\"site-nav\"><ul>{{pages}}</ul></nav>\n"
                "    </header>\n"
                "    <main>\n"
                "        <article>\n"
                "            <h1>{{title}}</h1>\n"
                "            <time>{{date}}</time>\n"
                "            {{content}}\n"
                "        </article>\n"
                "        <nav class=\"post-nav\">\n"
                "            <a class=\"prev\" href=\"{{prev_url}}\">{{prev_title}}</a>\n"
                "            <a class=\"next\" href=\"{{next_url}}\">{{next_title}}</a>\n"
                "        </nav>\n"
                "    </main>\n"
                "</body>\n"
                "</html>\n");
            fclose(f);
            printf("  created  %s/themes/%s/templates/post.html\n", name, themes[t]);
        }

        snprintf(path, sizeof(path), "%s/themes/%s/templates/page.html", name, themes[t]);
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
                "        <nav class=\"site-nav\"><ul>{{pages}}</ul></nav>\n"
                "    </header>\n"
                "    <main>\n"
                "        <article>\n"
                "            <h1>{{title}}</h1>\n"
                "            {{content}}\n"
                "        </article>\n"
                "    </main>\n"
                "</body>\n"
                "</html>\n");
            fclose(f);
            printf("  created  %s/themes/%s/templates/page.html\n", name, themes[t]);
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
            "    padding-left: 1rem; color: var(--muted); margin: 1.5rem 0; }\n"
            NAV_CSS);
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
            "    padding-left: 1rem; color: var(--muted); margin: 1.5rem 0; }\n"
            NAV_CSS);
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
            "    padding-left: 1rem; color: var(--muted); margin: 1.5rem 0; font-style: italic; }\n"
            NAV_CSS);
        fclose(f);
        printf("  created  %s/themes/sepia/static/style.css\n", name);
    }

    /* Example post, dated today rather than whenever this program was written */
    time_t     now = time(NULL);
    struct tm *tm  = localtime(&now);
    char       today[16];
    if (!tm || strftime(today, sizeof(today), "%Y-%m-%d", tm) == 0)
        snprintf(today, sizeof(today), "1970-01-01");

    snprintf(path, sizeof(path), "%s/content/posts/%s-hello-world.md", name, today);
    f = fopen(path, "w");
    if (f) {
        fprintf(f,
            "---\n"
            "title: Hello World\n"
            "date: %s\n"
            "slug: hello-world\n"
            "description: My first post\n"
            "draft: false\n"
            "---\n\n"
            "Welcome to my blog. This is the first post.\n",
            today);
        fclose(f);
        printf("  created  %s/content/posts/%s-hello-world.md\n", name, today);
    }

    /* Starter pages. Markdown at the top of content/ is published at /<slug>/
       and linked from the menu automatically. */
    int picked[PAGE_COUNT];
    int npicked = parse_page_choice(page_choice, picked);
    for (int i = 0; i < npicked; i++)
        write_page(name, picked[i], i + 1, author, desc);

    printf("\nDone! Next steps:\n\n");
    printf("  cd %s\n", name);
    printf("  atomik-ssg build\n");
    printf("  atomik-ssg serve\n\n");
}