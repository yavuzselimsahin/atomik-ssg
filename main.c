#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include "toml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmark.h>
#include <dirent.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define MAX_LINE 512
#define MAX_FIELD 256
#define SITE_TITLE "YSS Blog"
#define SITE_DESCRIPTION "Programming, Computer Science, Engineering"

TomlDoc g_toml;
char g_theme_path[512] = "themes/default";

void load_config(void)
{
	if (toml_parse("config.toml", &g_toml) != 0)
	{
		fprintf(stderr, "Warning: config.toml not found, using defaults\n");
		return;
	}

	const char *theme = toml_get_or(&g_toml, "", "theme", "default");
	snprintf(g_theme_path, sizeof(g_theme_path), "themes/%s", theme);

	DIR *d = opendir(g_theme_path);
	if (!d)
	{
		fprintf(stderr, "WarningL theme %s not found, falling back to default\n", theme);
		strncpy(g_theme_path, "themes/default", sizeof(g_theme_path) - 1);
	}
	else
	{
		closedir(d);
	}
}

typedef struct
{
	char title[MAX_FIELD];
	char date[MAX_FIELD];
	char slug[MAX_FIELD];
	char description[MAX_FIELD];
	char *content
} Post;

int parse_frontmatter(const char *raw, Post *post)
{
	if (strncmp(raw, "---", 3) != 0)
		return -1;

	const char *ptr = raw + 3;
	while (*ptr == '\r' || *ptr == '\n')
		ptr++;

	char line[MAX_LINE];
	while (*ptr)
	{

		int i = 0;
		while (*ptr && *ptr != '\n' && i < MAX_LINE - 1)
			line[i++] = *ptr++;
		line[i] = '\0';
		if (*ptr == '\n')
			ptr++;

		int llen = strlen(line);
		if (llen > 0 && line[llen - 1] == '\r')
			line[llen - 1] = '\0';

		if (strncmp(line, "---", 3) == 0)
		{
			while (*ptr == '\r' || *ptr == '\n')
				ptr++;
			post->content = (char *)ptr;
			return 0;
		}

		char *colon = strchr(line, ':');
		if (!colon)
			continue;

		*colon = '\0';
		char *key = line;
		char *val = colon + 1;
		while (*val == ' ')
			val++;

		char *cr = strchr(val, '\r');
		if (cr)
			*cr = '\0';

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

char *read_file(const char *path)
{

	FILE *f = fopen(path, "rb");
	if (!f)
	{
		perror(path);
		return NULL;
	}

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
        if (sub) {
            closedir(sub);
            copy_dir(src_path, dst_path);
        } else {
            copy_file(src_path, dst_path);
        }
    }
    closedir(d);
}

char *render_template(const char *tmpl, const Post *post, const char *html_content)
{
	size_t out_size = strlen(tmpl) + strlen(html_content) + 4096;
	char *out = malloc(out_size);
	if (!out)
		return NULL;

	const char *src = tmpl;
	char *dst = out;

	while (*src)
	{
		if (strncmp(src, "{{title}}", 9) == 0)
		{
			size_t len = strlen(post->title);
			memcpy(dst, post->title, len);
			dst += len;
			src += 9;
		}
		else if (strncmp(src, "{{date}}", 8) == 0)
		{
			size_t len = strlen(post->date);
			memcpy(dst, post->date, len);
			dst += len;
			src += 8;
		}
		else if (strncmp(src, "{{description}}", 15) == 0)
		{
			size_t len = strlen(post->description);
			memcpy(dst, post->description, len);
			dst += len;
			src += 15;
		}
		else if (strncmp(src, "{{content}}", 11) == 0)
		{
			size_t len = strlen(html_content);
			size_t used = dst - out;
			if (used + len + 1024 > out_size)
			{
				out_size = used + len + 4096;
				char *new_out = realloc(out, out_size);
				if (!new_out)
				{
					free(out);
					return NULL;
				}
				dst = new_out + used;
				out = new_out;
			}
			memcpy(dst, html_content, len);
			dst += len;
			src += 11;
		}
		else
		{
			*dst++ = *src++;
		}
	}
	*dst = '\0';
	return out;
}

int render_post(const char *md_path)
{
	char *raw = read_file(md_path);
	if (!raw)
		return -1;

	Post post = {0};
	if (parse_frontmatter(raw, &post) != 0)
	{
		fprintf(stderr, "Frontmatter error: %s\n", md_path);
		free(raw);
		return -1;
	}

	char *html_content = cmark_markdown_to_html(
		post.content, strlen(post.content), CMARK_OPT_DEFAULT);

	char tmpl_path[600];
	snprintf(tmpl_path, sizeof(tmpl_path), "%s/templates/post.html", g_theme_path);
	char *tmpl = read_file(tmpl_path);

	if (!tmpl)
	{
		free(raw);
		free(html_content);
		return -1;
	}

	char *output = render_template(tmpl, &post, html_content);

	char dir[512];
	snprintf(dir, sizeof(dir), "public/posts/%s", post.slug);
	mkdir("public", 0755);
	mkdir("public/posts", 0755);
	mkdir(dir, 0755);

	char out_path[512];
	snprintf(out_path, sizeof(out_path), "%s/index.html", dir);
	FILE *f = fopen(out_path, "wb");
	if (f)
	{
		fputs(output, f);
		fclose(f);
		printf("Generated: %s\n", out_path);
	}

	free(output);
	free(tmpl);
	free(html_content);
	free(raw);
	return 0;
}

void cmd_build(void)
{
	DIR *dir = opendir("content/posts");
	if (!dir)
	{
		fprintf(stderr, "Error: content/posts directory cannot found\n");
		fprintf(stderr, "atomik-ssg must initialized in a project directory \n");
		return;
	}

	mkdir("public", 0755);
	mkdir("public/posts", 0755);

	char post_items[8192] = {0};
	char item[1024];
	int count = 0;

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		char *ext = strrchr(entry->d_name, '.');
		if (!ext || strcmp(ext, ".md") != 0)
			continue;

		char path[512];
		snprintf(path, sizeof(path), "content/posts/%s", entry->d_name);
		if (render_post(path) == 0)
			count++;

		char *raw = read_file(path);
		if (!raw)
			continue;

		Post post = {0};
		if (parse_frontmatter(raw, &post) == 0)
		{
			snprintf(item, sizeof(item),
					 "		<li>\n"
					 "			<time>%s<time>\n"
					 "			<a href=\"/posts/%s/\">%s</a>\n"
					 "			<p>%s</p>\n"
					 "		</li>\n",
					 post.date, post.slug, post.title, post.description);
			strncat(post_items, item, sizeof(post_items) - strlen(post_items) - 1);
		}
		free(raw);
	}
	closedir(dir);

	char tmpl_path[600];
	snprintf(tmpl_path, sizeof(tmpl_path), "%s/templates/index.html", g_theme_path);
	char *tmpl = read_file(tmpl_path);

	if (!tmpl)
	{
		fprintf(stderr, "Error: templates/index.html can not be found\n");
		return;
	}

	Post site = {0};
	strncpy(site.title, toml_get_or(&g_toml, "", "title", "Atomik SSG"), MAX_FIELD - 1);
	strncpy(site.description, toml_get_or(&g_toml, "", "description", ""), MAX_FIELD - 1);

	char *output = render_template(tmpl, &site, "");
	char *pos = strstr(output, "{{post_items}}");
	if (pos)
	{
		size_t before = pos - output;
		size_t items_len = strlen(post_items);
		size_t after_off = before + 14;
		size_t total = before + items_len + strlen(output + after_off) + 1;

		char *final = malloc(total);
		memcpy(final, output, before);
		memcpy(final + before, post_items, items_len);
		strcpy(final + before + items_len, output + after_off);

		FILE *f = fopen("public/index.html", "wb");
		if (f)
		{
			fputs(final, f);
			fclose(f);
			printf("Generated: public/index.html\n");
		}
		free(final);
	}

	free(output);
	free(tmpl);
	char static_src[600];
	snprintf(static_src, sizeof(static_src), "%s/static", g_theme_path);
	copy_dir(static_src, "public");

	copy_dir("static", "public");
	printf("\nBuild completed: %d content generated -> public/\n", count);
}

void cmd_new(const char *title)
{
	if (!title)
	{
		fprintf(stderr, "Command: atomik-ssg new \"Content Title\"\n");
		return;
	}

	char slug[MAX_FIELD] = {0};
	int j = 0;
	for (int i = 0; title[i] && j < MAX_FIELD - 2; i++)
	{
		unsigned char c = (unsigned char)title[i];
		if (c >= 'A' && c <= 'Z')
		{
			slug[j++] = c + 32;
			continue;
		}
		if (c == ' ' || c == '_')
		{
			slug[j++] = '-';
			continue;
		}
		if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')
			slug[j++] = c;
	}
	slug[j] = '\0';

	char *slug_start = slug;
	while (*slug_start == '-')
		slug_start++;
	int slen = strlen(slug_start);
	while (slen > 0 && slug_start[slen - 1] == '-')
		slen--;
	slug_start[slen] = '\0';

	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char date[32];
	strftime(date, sizeof(date), "%Y-%m-%d", tm);

	char path[512];
	snprintf(path, sizeof(path), "content/posts/%s-%s.md", date, slug_start);

	FILE *check = fopen(path, "r");
	if (check)
	{
		fclose(check);
		fprintf(stderr, "Error: %s already exists\n", path);
		return;
	}

	FILE *f = fopen(path, "w");
	if (!f)
	{
		perror(path);
		return;
	}
	fprintf(f,
			"---\n"
			"title: %s\n"
			"date: %s\n"
			"slug: %s\n"
			"description: \n"
			"---\n\n"
			"Yazini buraya yaz...\n",
			title, date, slug_start);
	fclose(f);

	printf("Generated: %s\n", path);
}

const char *get_mime(const char *path)
{
	const char *ext = strrchr(path, '.');
	if (!ext)
		return "application/octet-stream";
	if (strcmp(ext, ".html") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(ext, ".css") == 0)
		return "text/css";
	if (strcmp(ext, ".js") == 0)
		return "application/javascript";
	if (strcmp(ext, ".png") == 0)
		return "image/png";
	if (strcmp(ext, ".jpg") == 0)
		return "image/jpeg";
	if (strcmp(ext, ".ico") == 0)
		return "image/x-icon";
	return "text/plain";
}

void send_response(int client, int status, const char *mime,
				   const char *body, size_t body_len)
{
	char header[512];
	const char *status_text = (status == 200) ? "OK" : "Not Found";
	snprintf(header, sizeof(header),
			 "HTTP/1.1 %d %s\r\n"
			 "Content-Type: %s\r\n"
			 "Content-Length: %zu\r\n"
			 "Connection: close\r\n"
			 "\r\n",
			 status, status_text, mime, body_len);
	send(client, header, strlen(header), 0);
	if (body && body_len > 0)
		send(client, body, body_len, 0);
}

static void prompt(const char *question, const char *fallback, char *out, int size)
{
	printf("%s", question);
	if (fallback && fallback[0])
		printf(" [%s]", fallback);
	printf(": ");
	fflush(stdout);

	if (!fgets(out, size, stdin))
	{
		out[0] = '\0';
		return;
	}

	/* Strip newline */
	char *nl = strchr(out, '\n');
	if (nl)
		*nl = '\0';
	char *cr = strchr(out, '\r');
	if (cr)
		*cr = '\0';

	/* Use fallback if empty */
	if (out[0] == '\0' && fallback)
		strncpy(out, fallback, size - 1);
}

void cmd_init(void)
{
	char name[256] = {0};
	char title[256] = {0};
	char desc[256] = {0};
	char author[256] = {0};
	char url[256] = {0};

	printf("\nWelcome to atomik-ssg!\n");
	printf("--------------------------\n\n");

	prompt("Project name", "my-blog", name, sizeof(name));
	prompt("Site title", "My Blog", title, sizeof(title));
	prompt("Description", "", desc, sizeof(desc));
	prompt("Author", "", author, sizeof(author));
	prompt("Base URL", "http://localhost", url, sizeof(url));

	printf("\nAvailable themes:\n");
	printf("  1) default  (light, minimal)\n");
	printf("  2) dark     (terminal feel)\n");
	printf("  3) sepia    (warm, book-like)\n\n");

	char theme_choice[32] = {0};
	prompt("Theme", "1", theme_choice, sizeof(theme_choice));

	char theme_name[32] = {0};
	if (strcmp(theme_choice, "2") == 0)
		strncpy(theme_name, "dark", sizeof(theme_name) - 1);
	else if (strcmp(theme_choice, "3") == 0)
		strncpy(theme_name, "sepia", sizeof(theme_name) - 1);
	else
		strncpy(theme_name, "default", sizeof(theme_name) - 1);

	printf("\n");

	/* Initialize project structure */
	if (mkdir(name, 0755) != 0)
	{
		fprintf(stderr, "Error: directory '%s' already exists\n", name);
		return;
	}

	/* sub paths */
	char path[512];
#define MKPATH(sub)                                   \
	snprintf(path, sizeof(path), "%s/%s", name, sub); \
	mkdir(path, 0755);

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
	MKPATH("public");
	MKPATH("static");
	MKPATH("static/images");

	/* config.toml */
	snprintf(path, sizeof(path), "%s/config.toml", name);
	FILE *f = fopen(path, "w");
	if (f)
	{
		fprintf(f,
				"title       = \"%s\"\n"
				"description = \"%s\"\n"
				"base_url    = \"%s\"\n"
				"author      = \"%s\"\n"
				"theme      = \"%s\"\n"
				"\n"
				"[build]\n"
				"output_dir = \"public\"\n"
				"\n"
				"[server]\n"
				"port = 4545\n",
				title, desc, url, author, theme_name);
		fclose(f);
		printf("  created  %s/config.toml\n", name);
	}

	/* templates/index.html */
	const char *themes[] = {"default", "dark", "sepia"};
	for (int t = 0; t < 3; t++)
	{
		/* index.html */
		snprintf(path, sizeof(path), "%s/themes/%s/templates/index.html", name, themes[t]);
		f = fopen(path, "w");
		if (f)
		{
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

		/* post.html */
		snprintf(path, sizeof(path), "%s/themes/%s/templates/post.html", name, themes[t]);
		f = fopen(path, "w");
		if (f)
		{
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
					"        <a href=\"/\">← Home</a>\n"
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

	/* static/style.css */
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

/* dark — terminal feel */
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
        "    margin-bottom: 2rem; padding-bottom: 1rem; border-bottom: 1px solid var(--border); }\n"
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

/* sepia — warm, book-like */
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

	/* Example content */
	snprintf(path, sizeof(path), "%s/content/posts/2026-08-10-hello-world.md", name);
	f = fopen(path, "w");
	if (f)
	{
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

void cmd_serve(int port)
{
#ifdef _WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

	int server_fd;
	struct sockaddr_in addr;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	/* Portu hemen yeniden kullan */
	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		fprintf(stderr, "Error: port %d is unavailable\n", port);
		return;
	}

	listen(server_fd, 10);
	printf("Server running: http://localhost:%d\n", port);
	printf("For exit Ctrl+C\n\n");

	char buf[2048];
	while (1)
	{
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		int client = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
		if (client < 0)
			continue;

		/* İsteği oku */
		int received = recv(client, buf, sizeof(buf) - 1, 0);
		if (received <= 0)
		{
#ifdef _WIN32
			closesocket(client);
#else
			close(client);
#endif
			continue;
		}
		buf[received] = '\0';

		/* GET /path HTTP/1.1 parse et */
		char method[8], url_path[512];
		sscanf(buf, "%7s %511s", method, url_path);
		printf("%s %s\n", method, url_path);

		/* / → /index.html */
		char file_path[600];
		if (strcmp(url_path, "/") == 0)
			snprintf(file_path, sizeof(file_path), "public/index.html");
		else if (url_path[strlen(url_path) - 1] == '/')
			snprintf(file_path, sizeof(file_path), "public%sindex.html", url_path);
		else
			snprintf(file_path, sizeof(file_path), "public%s", url_path);

		/* Dosyayı oku ve gönder */
		FILE *fp = fopen(file_path, "rb");
		if (fp) {
			fseek(fp, 0, SEEK_END);
			long size = ftell(fp);
			rewind(fp);

			char *body = malloc(size);
			fread(body, 1, size, fp);
			fclose(fp);

			send_response(client, 200, get_mime(file_path), body, size);
			free(body);
		} else {
			const char *not_found = "<h1>404 - Page not found</h1>";
			send_response(client, 404, "text/html", not_found, strlen(not_found));
		}

#ifdef _WIN32
		closesocket(client);
#else
		close(client);
#endif
	}

#ifdef _WIN32
	WSACleanup();
#endif
}

void print_help(void)
{
	printf("atomik-ssg - Lightweight static site generator\n\n");
	printf("Usage:\n");
	printf("  atomik-ssg init			Create new project\n");
	printf("  atomik-ssg build			Generate site\n");
	printf("  atomik-ssg new <title>		Generate new content\n");
	printf("  atomik-ssg serve [port]		Start dev server (deafult:4545)\n");
	printf("  atomik-ssg help			Show this message\n\n");
	printf("Example:\n");
	printf("  atomik-ssg new \"Programming linux kernel module\"\n");
}

int main(int argc, char *argv[])
{
	load_config();

	if (argc < 2)
	{
		print_help();
		return 0;
	}

	if (strcmp(argv[1], "build") == 0)
	{
		cmd_build();
	}
	else if (strcmp(argv[1], "new") == 0)
	{
		cmd_new(argc > 2 ? argv[2] : NULL);
	}
	else if (strcmp(argv[1], "help") == 0)
	{
		print_help();
	}
	else if (strcmp(argv[1], "serve") == 0)
	{
		int port = argc > 2 ? atoi(argv[2]) : 4545;
		cmd_serve(port);
	}
	else if (strcmp(argv[1], "init") == 0)
	{
		cmd_init();
	}
	else
	{
		fprintf(stderr, "Unknown command: %s\n", argv[1]);
		fprintf(stderr, "atomik-ssg help for more info\n");
		return 1;
	}

	return 0;
}
