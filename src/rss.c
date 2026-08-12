#include "../include/rss.h"
#include "../include/util.h"
#include "../include/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void generate_rss(const PostList *list, const char *outdir) {
    char out_path[512];
    snprintf(out_path, sizeof(out_path), "%s/rss.xml", outdir);

    FILE *f = fopen(out_path, "wb");
    if (!f) { perror(out_path); return; }

    /* A trailing slash on base_url would produce "//posts/..." links. */
    char base[512];
    snprintf(base, sizeof(base), "%s",
             toml_get_or(&g_toml, "", "base_url", "http://localhost"));
    size_t blen = strlen(base);
    while (blen > 0 && base[blen - 1] == '/') base[--blen] = '\0';

    char *title    = escape_html(toml_get_or(&g_toml, "", "title", "My Blog"));
    char *desc     = escape_html(toml_get_or(&g_toml, "", "description", ""));
    char *base_esc = escape_html(base);

    if (!title || !desc || !base_esc) {
        fprintf(stderr, "Error: out of memory generating RSS\n");
        free(title); free(desc); free(base_esc);
        fclose(f);
        return;
    }

    fprintf(f,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<rss version=\"2.0\" xmlns:atom=\"http://www.w3.org/2005/Atom\">\n"
        "  <channel>\n"
        "    <title>%s</title>\n"
        "    <link>%s</link>\n"
        "    <description>%s</description>\n"
        "    <language>en</language>\n"
        "    <atom:link href=\"%s/rss.xml\" rel=\"self\" type=\"application/rss+xml\" />\n\n",
        title, base_esc, desc, base_esc);

    for (int i = 0; i < list->count; i++) {
        const Post *p = &list->posts[i];

        char *ptitle = escape_html(p->title);
        char *pdesc  = escape_html(p->description);
        if (!ptitle || !pdesc) { free(ptitle); free(pdesc); continue; }

        char pubdate[64];
        int has_date = rfc822_date(p->date, pubdate, sizeof(pubdate)) == 0;
        if (!has_date && p->date[0])
            fprintf(stderr, "Warning: unparsable date \"%s\" in %s, omitting pubDate\n",
                    p->date, p->slug);

        fprintf(f,
            "    <item>\n"
            "      <title>%s</title>\n"
            "      <link>%s/posts/%s/</link>\n"
            "      <description>%s</description>\n",
            ptitle, base_esc, p->slug, pdesc);

        if (has_date)
            fprintf(f, "      <pubDate>%s</pubDate>\n", pubdate);

        fprintf(f,
            "      <guid isPermaLink=\"true\">%s/posts/%s/</guid>\n"
            "    </item>\n\n",
            base_esc, p->slug);

        free(ptitle);
        free(pdesc);
    }

    fprintf(f,
        "  </channel>\n"
        "</rss>\n");

    free(title);
    free(desc);
    free(base_esc);

    if (fclose(f) != 0) { fprintf(stderr, "Error: could not write %s\n", out_path); return; }
    printf("Generated: %s\n", out_path);
}
