#include "../include/rss.h"
#include "../toml.h"
#include <stdio.h>
#include <string.h>

extern TomlDoc g_toml;

void generate_rss(const PostList *list) {
    FILE *f = fopen("public/rss.xml", "wb");
    if (!f) { fprintf(stderr, "Error: cannot write rss.xml\n"); return; }

    const char *title    = toml_get_or(&g_toml, "", "title",       "My Blog");
    const char *desc     = toml_get_or(&g_toml, "", "description", "");
    const char *base_url = toml_get_or(&g_toml, "", "base_url",    "http://localhost");

    fprintf(f,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<rss version=\"2.0\">\n"
        "  <channel>\n"
        "    <title>%s</title>\n"
        "    <link>%s</link>\n"
        "    <description>%s</description>\n"
        "    <language>en</language>\n\n",
        title, base_url, desc);

    for (int i = 0; i < list->count; i++) {
        const Post *p = &list->posts[i];
        fprintf(f,
            "    <item>\n"
            "      <title>%s</title>\n"
            "      <link>%s/posts/%s/</link>\n"
            "      <description>%s</description>\n"
            "      <pubDate>%s</pubDate>\n"
            "      <guid>%s/posts/%s/</guid>\n"
            "    </item>\n\n",
            p->title, base_url, p->slug,
            p->description,
            p->date,
            base_url, p->slug);
    }

    fprintf(f,
        "  </channel>\n"
        "</rss>\n");

    fclose(f);
    printf("Generated: public/rss.xml\n");
}