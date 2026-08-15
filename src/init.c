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
    ".post-nav .next::after { content: \"  \\2192\"; }\n" \
    "/* The label is CSS, not markup, so a theme decides how it reads. */\n" \
    ".callout { border-left: 3px solid var(--accent);\n" \
    "    background: var(--code-bg); padding: 0.8rem 1rem;\n" \
    "    margin: 1.5rem 0; }\n" \
    ".callout::before { display: block; font-size: 0.72rem;\n" \
    "    font-weight: 700; text-transform: uppercase;\n" \
    "    letter-spacing: 0.06em; margin-bottom: 0.3rem;\n" \
    "    color: var(--accent); }\n" \
    ".callout > :last-child { margin-bottom: 0; }\n" \
    ".callout-note::before { content: \"Note\"; }\n" \
    ".callout-tip::before { content: \"Tip\"; }\n" \
    ".callout-important::before { content: \"Important\"; }\n" \
    ".callout-warning::before { content: \"Warning\"; }\n" \
    ".callout-caution::before { content: \"Caution\"; }\n" \
    "/* Empty when built_with is off, and then it takes up no room. */\n" \
    ".site-footer { margin-top: 3rem; padding-top: 1.1rem;\n" \
    "    border-top: 1px solid var(--border); font-size: 0.78rem;\n" \
    "    line-height: 1.5; letter-spacing: 0.01em; color: var(--muted); }\n" \
    ".site-footer:empty { display: none; }\n" \
    ".site-footer a { color: var(--muted); text-decoration: none; }\n" \
    ".site-footer a:hover { color: var(--accent); }\n"

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

/* ------------------------------------------------------------------ *
 * The docs theme.
 *
 * Unlike the three blog themes it needs its own templates, not just its own
 * colours: a sidebar carrying {{page_tree}} sits beside the content on every
 * kind of page. These strings contain no substitutions, so they are written
 * with fputs — a stray % in the CSS is a format specifier to fprintf.
 * ------------------------------------------------------------------ */

/* Shared by all three docs templates.

   The top bar carries only the site name. The sidebar already lists every
   section, so repeating them up here would just compete with it. */
#define DOCS_SHELL_HEAD \
    "<!DOCTYPE html>\n" \
    "<html lang=\"en\">\n" \
    "<head>\n" \
    "    <meta charset=\"UTF-8\">\n" \
    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n" \
    "    <title>{{title}}</title>\n" \
    "    <meta name=\"description\" content=\"{{description}}\">\n" \
    "    <link rel=\"stylesheet\" href=\"{{base_path}}/style.css\">\n" \
    "    <script>\n" \
    "    /* Runs before the first paint, so a reader who chose a scheme never\n" \
    "       sees the other one flash first. */\n" \
    "    (function () {\n" \
    "        var t = localStorage.getItem('theme');\n" \
    "        if (t) document.documentElement.setAttribute('data-theme', t);\n" \
    "    })();\n" \
    "    </script>\n" \
    "</head>\n" \
    "<body>\n" \
    "    <header class=\"topbar\">\n" \
    "        <a class=\"brand\" href=\"{{base_path}}/\">{{site_title}}</a>\n" \
    "        <span class=\"version\">{{version}}</span>\n" \
    "        <button class=\"theme-toggle\" type=\"button\"\n" \
    "                aria-label=\"Switch between light and dark\">\n" \
    "            <span class=\"in-light\">Dark</span>\n" \
    "            <span class=\"in-dark\">Light</span>\n" \
    "        </button>\n" \
    "    </header>\n" \
    "    <div class=\"layout\">\n" \
    "        <aside class=\"sidebar\">\n" \
    "            {{page_tree}}\n" \
    "            <footer class=\"site-footer\">{{built_with}}</footer>\n" \
    "        </aside>\n" \
    "        <main>\n"

/* Marks the entry the reader is on. The tree is built once for the whole site,
   so the one thing it cannot know is which page it ended up in. */
#define DOCS_SHELL_TAIL \
    "        </main>\n" \
    "    </div>\n" \
    "    <script>\n" \
    "    (function () {\n" \
    "        var here = location.pathname;\n" \
    "        document.querySelectorAll('.sidebar > ul a').forEach(function (a) {\n" \
    "            if (a.getAttribute('href') === here) a.classList.add('active');\n" \
    "        });\n" \
    "\n" \
    "        var root = document.documentElement;\n" \
    "        var button = document.querySelector('.theme-toggle');\n" \
    "        if (button) button.addEventListener('click', function () {\n" \
    "            /* No stored choice yet means we are following the system. */\n" \
    "            var now = root.getAttribute('data-theme') ||\n" \
    "                (matchMedia('(prefers-color-scheme: dark)').matches\n" \
    "                    ? 'dark' : 'light');\n" \
    "            var next = now === 'dark' ? 'light' : 'dark';\n" \
    "            root.setAttribute('data-theme', next);\n" \
    "            localStorage.setItem('theme', next);\n" \
    "        });\n" \
    "    })();\n" \
    "    </script>\n" \
    "</body>\n" \
    "</html>\n"

#define DOCS_FOOTER \
    "            <nav class=\"post-nav\">\n" \
    "                <a class=\"prev\" href=\"{{prev_url}}\">{{prev_title}}</a>\n" \
    "                <a class=\"next\" href=\"{{next_url}}\">{{next_title}}</a>\n" \
    "            </nav>\n" \
    "            <a class=\"edit\" href=\"{{edit_url}}\">Edit this page</a>\n"

/* A documentation landing page is a table of contents, not a feed. Listing
   {{page_tree}} keeps it correct no matter which pages exist, where a
   hand-written "start here" link would rot the moment one is renamed. */
static const char DOCS_INDEX[] =
    DOCS_SHELL_HEAD
    "            <article>\n"
    "                <h1>{{site_title}}</h1>\n"
    "                <p class=\"lede\">{{site_description}}</p>\n"
    "            </article>\n"
    "            <nav class=\"contents\">\n"
    "                <h2>Contents</h2>\n"
    "                {{page_tree}}\n"
    "            </nav>\n"
    DOCS_SHELL_TAIL;

static const char DOCS_PAGE[] =
    DOCS_SHELL_HEAD
    "            <article>\n"
    "                <h1>{{title}}</h1>\n"
    "                {{content}}\n"
    "            </article>\n"
    DOCS_FOOTER
    DOCS_SHELL_TAIL;

static const char DOCS_POST[] =
    DOCS_SHELL_HEAD
    "            <article>\n"
    "                <h1>{{title}}</h1>\n"
    "                <time>{{date}}</time>\n"
    "                {{content}}\n"
    "            </article>\n"
    DOCS_FOOTER
    DOCS_SHELL_TAIL;

static const char DOCS_CSS[] =
    "/* Light is the palette of the default blog theme, dark is the palette of\n"
    "   the dark one, so a site that mixes docs and posts stays one thing.\n"
    "   --panel is the only addition: the docs layout has raised surfaces\n"
    "   (hover rows, table headers, callouts) that the blog templates lack. */\n"
    ":root {\n"
    "    --bg:       #ffffff;\n"
    "    --panel:    #f7fafc;\n"
    "    --border:   #e2e8f0;\n"
    "    --text:     #1a202c;\n"
    "    --muted:    #718096;\n"
    "    --accent:   #2b6cb0;\n"
    "    --code-bg:  #f7fafc;\n"
    "    --cal-note:      #2b6cb0;\n"
    "    --cal-tip:       #276749;\n"
    "    --cal-important: #553c9a;\n"
    "    --cal-warning:   #975a16;\n"
    "    --cal-caution:   #9b2c2c;\n"
    "}\n"
    "/* Dark applies when the system asks for it and the reader has not chosen\n"
    "   otherwise, or when the reader chose it outright. The two blocks carry\n"
    "   the same values; only the condition differs. */\n"
    "@media (prefers-color-scheme: dark) {\n"
    "    :root:not([data-theme=\"light\"]) {\n"
    "        --bg:      #0f1117;\n"
    "        --panel:   #191d28;\n"
    "        --border:  #2a2d3a;\n"
    "        --text:    #e2e8f0;\n"
    "        --muted:   #8b98ab;\n"
    "        --accent:  #60a5fa;\n"
    "        --code-bg: #141720;\n"
    "        --cal-note:      #63b3ed;\n"
    "        --cal-tip:       #68d391;\n"
    "        --cal-important: #b794f4;\n"
    "        --cal-warning:   #f6ad55;\n"
    "        --cal-caution:   #fc8181;\n"
    "    }\n"
    "}\n"
    ":root[data-theme=\"dark\"] {\n"
    "    --bg:      #0f1117;\n"
    "    --panel:   #191d28;\n"
    "    --border:  #2a2d3a;\n"
    "    --text:    #e2e8f0;\n"
    "    --muted:   #8b98ab;\n"
    "    --accent:  #60a5fa;\n"
    "    --code-bg: #141720;\n"
    "    --cal-note:      #63b3ed;\n"
    "    --cal-tip:       #68d391;\n"
    "    --cal-important: #b794f4;\n"
    "    --cal-warning:   #f6ad55;\n"
    "    --cal-caution:   #fc8181;\n"
    "}\n"
    "* { box-sizing: border-box; margin: 0; padding: 0; }\n"
    "body { background: var(--bg); color: var(--text); line-height: 1.7;\n"
    "    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto,\n"
    "        Helvetica, Arial, sans-serif; }\n"
    "\n"
    "/* top bar */\n"
    ".topbar { display: flex; align-items: center; justify-content: space-between;\n"
    "    gap: 0.5rem; padding: 0.9rem 1.5rem;\n"
    "    border-bottom: 1px solid var(--border);\n"
    "    position: sticky; top: 0; background: var(--bg); z-index: 10; }\n"
    ".brand { font-weight: 600; color: var(--text); text-decoration: none; }\n"
    "/* Plain text beside the site name, not a badge. margin-right pushes\n"
    "   everything after it to the far end. Empty when config.toml sets no\n"
    "   version, and then it is not there at all. */\n"
    ".version { margin-right: auto; font-size: 0.78rem; color: var(--muted); }\n"
    ".version:empty { display: none; }\n"
    ".theme-toggle { font: inherit; font-size: 0.8rem; cursor: pointer;\n"
    "    color: var(--muted); background: none; padding: 0.25rem 0.6rem;\n"
    "    border: 1px solid var(--border); border-radius: 5px; }\n"
    ".theme-toggle:hover { color: var(--accent); border-color: var(--accent); }\n"
    "/* Each label shows only in the scheme it switches away from. */\n"
    ".theme-toggle .in-dark { display: none; }\n"
    "@media (prefers-color-scheme: dark) {\n"
    "    :root:not([data-theme=\"light\"]) .theme-toggle .in-light { display: none; }\n"
    "    :root:not([data-theme=\"light\"]) .theme-toggle .in-dark { display: inline; }\n"
    "}\n"
    ":root[data-theme=\"dark\"] .theme-toggle .in-light { display: none; }\n"
    ":root[data-theme=\"dark\"] .theme-toggle .in-dark { display: inline; }\n"
    ":root[data-theme=\"light\"] .theme-toggle .in-light { display: inline; }\n"
    ":root[data-theme=\"light\"] .theme-toggle .in-dark { display: none; }\n"
    "\n"
    "/* two columns, one on a phone */\n"
    ".layout { display: grid; grid-template-columns: 240px minmax(0, 1fr);\n"
    "    gap: 3rem; max-width: 1100px; margin: 0 auto; padding: 2rem 1.5rem; }\n"
    "\n"
    "/* sidebar */\n"
    "/* Two parts: the tree takes the height that is going and scrolls, the\n"
    "   footer under it stays where it is. Reading the attribution should not\n"
    "   mean scrolling an article to the end. */\n"
    ".sidebar { position: sticky; top: 4.5rem; align-self: start;\n"
    "    max-height: calc(100vh - 6rem); font-size: 0.9rem;\n"
    "    display: flex; flex-direction: column; }\n"
    ".sidebar > ul { overflow-y: auto; flex: 1; min-height: 0; }\n"
    ".sidebar ul { list-style: none; }\n"
    ".sidebar > ul > li { margin-bottom: 1.2rem; }\n"
    ".sidebar ul ul { margin: 0.3rem 0 0 0; padding-left: 0.8rem;\n"
    "    border-left: 1px solid var(--border); }\n"
    ".sidebar ul ul li { margin: 0.15rem 0; }\n"
    "/* Scoped to the tree: the attribution below it is an ordinary link, not\n"
    "   another row in the navigation. */\n"
    ".sidebar > ul a, .sidebar > ul span { display: block;\n"
    "    padding: 0.2rem 0.5rem; border-radius: 4px;\n"
    "    text-decoration: none; color: var(--muted); }\n"
    ".sidebar > ul > li > a, .sidebar > ul > li > span {\n"
    "    color: var(--text); font-weight: 600; }\n"
    ".sidebar > ul a:hover { background: var(--panel); color: var(--accent); }\n"
    ".sidebar > ul a.active { background: var(--panel); color: var(--accent);\n"
    "    font-weight: 600; }\n"
    "\n"
    "/* content */\n"
    "main { min-width: 0; }\n"
    "article h1 { font-size: 2rem; line-height: 1.25; margin-bottom: 1.2rem; }\n"
    "article h2 { font-size: 1.35rem; margin: 2.2rem 0 0.8rem;\n"
    "    padding-top: 0.6rem; border-top: 1px solid var(--border); }\n"
    "article h3 { font-size: 1.1rem; margin: 1.6rem 0 0.5rem; }\n"
    "article p, article ul, article ol { margin-bottom: 1.1rem; }\n"
    "article ul, article ol { padding-left: 1.4rem; }\n"
    "article a { color: var(--accent); }\n"
    "article time { display: block; color: var(--muted); font-size: 0.85rem;\n"
    "    margin: -0.6rem 0 1.6rem; }\n"
    ".lede { color: var(--muted); font-size: 1.1rem; }\n"
    "table { border-collapse: collapse; width: 100%; margin-bottom: 1.2rem;\n"
    "    display: block; overflow-x: auto; }\n"
    "th, td { border: 1px solid var(--border); padding: 0.5rem 0.8rem;\n"
    "    text-align: left; }\n"
    "th { background: var(--panel); }\n"
    "pre { background: var(--code-bg); border: 1px solid var(--border);\n"
    "    border-radius: 6px; padding: 1rem; overflow-x: auto;\n"
    "    margin-bottom: 1.2rem; font-size: 0.88rem; line-height: 1.5; }\n"
    "code { font-family: ui-monospace, SFMono-Regular, Menlo, monospace;\n"
    "    font-size: 0.88em; }\n"
    ":not(pre) > code { background: var(--code-bg); padding: 0.15em 0.4em;\n"
    "    border-radius: 4px; }\n"
    "blockquote { border-left: 3px solid var(--accent); background: var(--panel);\n"
    "    padding: 0.8rem 1rem; margin-bottom: 1.2rem; color: var(--muted); }\n"
    "\n"
    "/* table of contents on the landing page */\n"
    ".contents { margin-top: 2.5rem; }\n"
    ".contents h2 { font-size: 0.85rem; text-transform: uppercase;\n"
    "    letter-spacing: 0.06em; color: var(--muted); margin-bottom: 0.8rem; }\n"
    ".contents ul { list-style: none; }\n"
    ".contents > ul > li { padding: 0.7rem 0;\n"
    "    border-bottom: 1px solid var(--border); }\n"
    ".contents > ul > li > a, .contents > ul > li > span {\n"
    "    font-weight: 600; font-size: 1.05rem; color: var(--text);\n"
    "    text-decoration: none; }\n"
    ".contents > ul > li > a:hover { color: var(--accent); }\n"
    ".contents ul ul { display: flex; flex-wrap: wrap; gap: 0.3rem 1.2rem;\n"
    "    margin-top: 0.35rem; }\n"
    ".contents ul ul a { color: var(--muted); text-decoration: none;\n"
    "    font-size: 0.9rem; }\n"
    ".contents ul ul a:hover { color: var(--accent); }\n"
    "\n"
    "/* footer navigation */\n"
    ".post-nav { display: flex; gap: 1rem; margin-top: 3rem; }\n"
    ".post-nav a { flex: 1; max-width: 50%; padding: 0.8rem 1rem;\n"
    "    border: 1px solid var(--border); border-radius: 6px;\n"
    "    color: var(--accent); text-decoration: none; font-size: 0.9rem; }\n"
    ".post-nav a:hover { background: var(--panel); }\n"
    ".post-nav a:empty { display: none; }\n"
    ".post-nav .prev::before { content: \"\\2190  \"; }\n"
    ".post-nav .next { margin-left: auto; text-align: right; }\n"
    ".post-nav .next::after { content: \"  \\2192\"; }\n"
    "/* empty when config.toml sets no edit_url */\n"
    ".edit { display: inline-block; margin-top: 2rem; font-size: 0.85rem;\n"
    "    color: var(--muted); }\n"
    ".edit[href=\"\"] { display: none; }\n"
    "\n"
    "/* Callouts. The label is CSS, not markup. */\n"
    ".callout { border-left: 3px solid var(--border);\n"
    "    background: var(--panel); padding: 0.9rem 1rem;\n"
    "    margin-bottom: 1.2rem; border-radius: 0 5px 5px 0; }\n"
    ".callout::before { display: block; font-size: 0.72rem;\n"
    "    font-weight: 700; text-transform: uppercase;\n"
    "    letter-spacing: 0.06em; margin-bottom: 0.35rem; }\n"
    ".callout > :last-child { margin-bottom: 0; }\n"
    ".callout-note { border-left-color: var(--cal-note); }\n"
    ".callout-note::before { content: \"Note\"; color: var(--cal-note); }\n"
    ".callout-tip { border-left-color: var(--cal-tip); }\n"
    ".callout-tip::before { content: \"Tip\"; color: var(--cal-tip); }\n"
    ".callout-important { border-left-color: var(--cal-important); }\n"
    ".callout-important::before { content: \"Important\"; color: var(--cal-important); }\n"
    ".callout-warning { border-left-color: var(--cal-warning); }\n"
    ".callout-warning::before { content: \"Warning\"; color: var(--cal-warning); }\n"
    ".callout-caution { border-left-color: var(--cal-caution); }\n"
    ".callout-caution::before { content: \"Caution\"; color: var(--cal-caution); }\n"
    "\n"
    "/* Pinned to the foot of the sidebar, outside the part that scrolls.\n"
    "   No rule above it: in a column this narrow the gap says enough.\n"
    "   Empty when built_with is off, and then it takes no room. */\n"
    ".site-footer { flex-shrink: 0; margin-top: 1.4rem; font-size: 0.75rem;\n"
    "    line-height: 1.4; letter-spacing: 0.01em; color: var(--muted); }\n"
    ".site-footer:empty { display: none; }\n"
    ".site-footer a { color: var(--muted); text-decoration: none; }\n"
    ".site-footer a:hover { color: var(--accent); }\n"
    "\n"
    "/* Phone layout. This block has to come last: it unsets the sticky\n"
    "   positioning above, and at equal specificity the later rule wins. */\n"
    "@media (max-width: 800px) {\n"
    "    .layout { grid-template-columns: 1fr; gap: 1.5rem; padding-top: 1.5rem; }\n"
    "    .sidebar { position: static; max-height: none; overflow: visible;\n"
    "        border-bottom: 1px solid var(--border);\n"
    "        padding-bottom: 1.2rem; margin-bottom: 0.5rem; }\n"
    "    .sidebar > ul { overflow: visible; }\n"
    "    .sidebar > ul > li { margin-bottom: 0.9rem; }\n"
    "    .site-footer { margin-top: 0.9rem; }\n"
    "    article h1 { font-size: 1.6rem; }\n"
    "    .post-nav { flex-direction: column; }\n"
    "    .post-nav a { max-width: none; }\n"
    "    .post-nav .next { margin-left: 0; }\n"
    "}\n";

/* Starter documentation. It is written for the person who just ran init, and
   it doubles as a worked example of sections, ordering and index.md. Note the
   absence of pipe tables: the bundled cmark is strict CommonMark, so a table
   here would render as literal text. */
static const struct {
    const char *path;
    const char *body;
} DOCS_CONTENT[] = {
{ "getting-started.md",
  "---\n"
  "title: Getting Started\n"
  "description: Build this site and add your first page\n"
  "order: 1\n"
  "---\n\n"
  "Everything on this site is a markdown file under `content/`, and every entry\n"
  "in the sidebar is one of those files. Nothing registers pages anywhere else.\n\n"
  "## Build and preview\n\n"
  "```bash\n"
  "atomik-ssg build\n"
  "atomik-ssg serve\n"
  "```\n\n"
  "`build` writes the site into `public/`. `serve` hosts that directory at\n"
  "<http://localhost:4545>. Run `build` again after each change and refresh.\n\n"
  "## Add a page\n\n"
  "Create `content/install.md`:\n\n"
  "```markdown\n"
  "---\n"
  "title: Installation\n"
  "order: 2\n"
  "---\n\n"
  "Your content here.\n"
  "```\n\n"
  "Build, and it appears in the sidebar at `/install/`.\n\n"
  "## Where to go next\n\n"
  "- [Pages and Sections](/writing/pages/) turns files into URLs\n"
  "- [Navigation](/writing/navigation/) controls the order things appear in\n"
  "- [Reference](/reference/) lists the commands, frontmatter and variables\n" },

{ "writing/index.md",
  "---\n"
  "title: Writing Docs\n"
  "description: How content is organised\n"
  "order: 2\n"
  "---\n\n"
  "This section covers how markdown files become pages and how you control the\n"
  "shape of the sidebar.\n\n"
  "A directory becomes a section. Giving it an `index.md` — as this one has —\n"
  "makes the section itself a page; leave it out and the section is only a\n"
  "heading over its children.\n" },

{ "writing/pages.md",
  "---\n"
  "title: Pages and Sections\n"
  "description: How files map to URLs\n"
  "order: 1\n"
  "---\n\n"
  "A file's location is its URL. There is no routing table.\n\n"
  "- `content/install.md` becomes `/install/`\n"
  "- `content/writing/pages.md` becomes `/writing/pages/`\n"
  "- `content/writing/index.md` becomes `/writing/` itself\n\n"
  "Directories nest as deep as you need.\n\n"
  "## Slugs\n\n"
  "The URL comes from the file name, lowercased and stripped of anything that\n"
  "is not a letter, digit or dash. Accented and non-Latin letters are\n"
  "transliterated, so `Yapılandırma.md` becomes `/yapilandirma/`.\n\n"
  "Set `slug:` in the frontmatter to override the last part of the path:\n\n"
  "```markdown\n"
  "---\n"
  "title: Installation\n"
  "slug: install\n"
  "---\n"
  "```\n\n"
  "## Drafts\n\n"
  "`draft: true` keeps a page out of the build entirely — no page, no sidebar\n"
  "entry. Preview drafts with `atomik-ssg build --drafts`.\n\n"
  "## Blog posts\n\n"
  "Markdown under `content/posts/` is a dated post instead of a page: it is\n"
  "listed chronologically and carries an RSS entry. A documentation site can\n"
  "ignore that directory, or delete it.\n" },

{ "writing/navigation.md",
  "---\n"
  "title: Navigation\n"
  "description: Ordering the sidebar\n"
  "order: 2\n"
  "---\n\n"
  "The sidebar is derived from `content/`, so adding a file is all it takes to\n"
  "publish it. What you do control is the order.\n\n"
  "## Ordering\n\n"
  "Pages without an `order:` are sorted alphabetically by title. Add `order:`\n"
  "to the ones whose position matters and they move to the front, lowest\n"
  "first:\n\n"
  "```markdown\n"
  "---\n"
  "title: Getting Started\n"
  "order: 1\n"
  "---\n"
  "```\n\n"
  "Ordering applies at each level independently, so a section's children are\n"
  "arranged among themselves.\n\n"
  "## Previous and next\n\n"
  "Every page carries links to its neighbours, and those follow the sidebar\n"
  "from top to bottom — the reading order, not the file system or a calendar.\n"
  "Reordering the sidebar reorders them with it.\n\n"
  "## The top bar\n\n"
  "The menu across the top lists only top-level entries, so it stays short as\n"
  "the tree grows.\n" },

{ "reference/index.md",
  "---\n"
  "title: Reference\n"
  "description: Commands, frontmatter and template variables\n"
  "order: 3\n"
  "---\n\n"
  "The details, once you know your way around.\n" },

{ "reference/commands.md",
  "---\n"
  "title: Commands\n"
  "description: The command line\n"
  "order: 1\n"
  "---\n\n"
  "- `atomik-ssg init` scaffolds a new project\n"
  "- `atomik-ssg build` generates the site into `public/`\n"
  "- `atomik-ssg build --drafts` includes pages marked `draft: true`\n"
  "- `atomik-ssg serve` serves `public/` at <http://localhost:4545>\n"
  "- `atomik-ssg serve 8080` serves it on another port\n"
  "- `atomik-ssg new My Post Title` creates a dated post\n"
  "- `atomik-ssg deploy` builds, then mirrors the output over rsync\n"
  "- `atomik-ssg help` prints all of this\n" },

{ "reference/frontmatter.md",
  "---\n"
  "title: Frontmatter\n"
  "description: The fields a page can set\n"
  "order: 2\n"
  "---\n\n"
  "Every field is optional.\n\n"
  "- `title` — shown as the heading and in the sidebar. Falls back to the slug\n"
  "- `description` — used for the page's meta description\n"
  "- `slug` — overrides the URL segment taken from the file name\n"
  "- `order` — position in the sidebar. Unset means alphabetical, after the\n"
  "  pages that set it\n"
  "- `draft` — `true` keeps the page out of the build\n"
  "- `date` — posts only. Taken from the file name when omitted\n" },

{ "reference/templates.md",
  "---\n"
  "title: Template Variables\n"
  "description: What a theme can place\n"
  "order: 3\n"
  "---\n\n"
  "Templates live in `themes/<name>/templates/`. Any `{{name}}` below is\n"
  "replaced; anything unrecognised is left alone.\n\n"
  "- `{{title}}`, `{{description}}`, `{{slug}}`, `{{date}}` — this page\n"
  "- `{{content}}` — the rendered markdown\n"
  "- `{{site_title}}`, `{{site_description}}` — from `config.toml`\n"
  "- `{{page_tree}}` — the whole hierarchy, as a nested list\n"
  "- `{{pages}}` — top-level entries only, for a menu bar\n"
  "- `{{post_items}}` — the chronological post list, on the index\n"
  "- `{{prev_url}}`, `{{prev_title}}`, `{{next_url}}`, `{{next_title}}`\n"
  "- `{{source_path}}` — the markdown file this page came from\n"
  "- `{{edit_url}}` — that path joined to `edit_url` in `config.toml`\n\n"
  "Set `edit_url` to put an edit link on every page:\n\n"
  "```toml\n"
  "edit_url = \"https://github.com/you/project/edit/main\"\n"
  "```\n\n"
  "It is empty when unset, and the theme hides the link rather than showing a\n"
  "dead one.\n" },
};

#define DOCS_CONTENT_COUNT ((int)(sizeof(DOCS_CONTENT) / sizeof(DOCS_CONTENT[0])))

static void write_text(const char *path, const char *body, const char *label) {
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); return; }
    fputs(body, f);
    fclose(f);
    printf("  created  %s\n", label);
}

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
    printf("  3) sepia    (warm, book-like)\n");
    printf("  4) docs     (sidebar, for project documentation)\n\n");
    prompt("Theme", "1", theme_choice, sizeof(theme_choice));

    if (strcmp(theme_choice, "2") == 0)
        snprintf(theme_name, sizeof(theme_name), "dark");
    else if (strcmp(theme_choice, "3") == 0)
        snprintf(theme_name, sizeof(theme_name), "sepia");
    else if (strcmp(theme_choice, "4") == 0)
        snprintf(theme_name, sizeof(theme_name), "docs");
    else
        snprintf(theme_name, sizeof(theme_name), "default");

    int is_docs = strcmp(theme_name, "docs") == 0;

    /* A documentation site gets a starter set of docs instead; asking it
       whether it wants an About or a Now page would only be noise. */
    if (is_docs) {
        printf("\nThe docs theme starts you off with a short guide to writing\n");
        printf("documentation with atomik-ssg, which you can edit or delete.\n");
    } else {
        printf("\nStarter pages (these become the site menu, in the order you list them):\n");
        for (int i = 0; i < PAGE_COUNT; i++)
            printf("  %d) %-9s %s\n", i + 1, PAGE_TEMPLATES[i].slug, PAGE_TEMPLATES[i].summary);
        printf("\n");
        prompt("Pages, comma separated", "about", page_choice, sizeof(page_choice));
    }

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
    MKPATH("themes/docs");
    MKPATH("themes/docs/templates");
    MKPATH("themes/docs/static");
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
            "# Shown by the docs theme so a reader can tell which release this\n"
            "# documents. Leave it out and nothing is displayed.\n"
            "# version = \"1.0.0\"\n\n"
            "# Serving the site from a subdirectory rather than a domain root?\n"
            "# Set it here and every generated link is prefixed to match.\n"
            "# base_path = \"/my-project\"\n\n"
            "# A quiet line in the footer noting how the site was made.\n"
            "# Comment it out to remove it.\n"
            "built_with  = true\n\n"
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
                "    <link rel=\"stylesheet\" href=\"{{base_path}}/style.css\">\n"
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
                "    <footer class=\"site-footer\">{{built_with}}</footer>\n"
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
                "    <link rel=\"stylesheet\" href=\"{{base_path}}/style.css\">\n"
                "</head>\n"
                "<body>\n"
                "    <header>\n"
                "        <a href=\"{{base_path}}/\">&larr; Home</a>\n"
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
                "    <footer class=\"site-footer\">{{built_with}}</footer>\n"
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
                "    <link rel=\"stylesheet\" href=\"{{base_path}}/style.css\">\n"
                "</head>\n"
                "<body>\n"
                "    <header>\n"
                "        <a href=\"{{base_path}}/\">&larr; Home</a>\n"
                "        <nav class=\"site-nav\"><ul>{{pages}}</ul></nav>\n"
                "    </header>\n"
                "    <main>\n"
                "        <article>\n"
                "            <h1>{{title}}</h1>\n"
                "            {{content}}\n"
                "        </article>\n"
                "    </main>\n"
                "    <footer class=\"site-footer\">{{built_with}}</footer>\n"
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

    /* docs theme: its own templates, not just its own palette */
    {
        struct { const char *file; const char *body; } docs[] = {
            { "templates/index.html", DOCS_INDEX },
            { "templates/page.html",  DOCS_PAGE  },
            { "templates/post.html",  DOCS_POST  },
            { "static/style.css",     DOCS_CSS   },
        };
        for (size_t i = 0; i < sizeof(docs) / sizeof(docs[0]); i++) {
            char label[600];
            snprintf(path,  sizeof(path),  "%s/themes/docs/%s", name, docs[i].file);
            snprintf(label, sizeof(label), "%s/themes/docs/%s", name, docs[i].file);
            write_text(path, docs[i].body, label);
        }
    }

    if (is_docs) {
        for (int i = 0; i < DOCS_CONTENT_COUNT; i++) {
            char label[600];
            snprintf(path, sizeof(path), "%s/content/%s", name, DOCS_CONTENT[i].path);

            /* reference/commands.md needs content/reference/ to exist first */
            char *slash = strrchr(path, '/');
            if (slash) {
                *slash = '\0';
                if (make_dir_p(path) != 0) { perror(path); return; }
                *slash = '/';
            }

            snprintf(label, sizeof(label), "%s/content/%s", name, DOCS_CONTENT[i].path);
            write_text(path, DOCS_CONTENT[i].body, label);
        }
    } else {
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

        /* Starter pages. Markdown at the top of content/ is published at
           /<slug>/ and linked from the menu automatically. */
        int picked[PAGE_COUNT];
        int npicked = parse_page_choice(page_choice, picked);
        for (int i = 0; i < npicked; i++)
            write_page(name, picked[i], i + 1, author, desc);
    }

    printf("\nDone! Next steps:\n\n");
    printf("  cd %s\n", name);
    printf("  atomik-ssg build\n");
    printf("  atomik-ssg serve\n\n");
}