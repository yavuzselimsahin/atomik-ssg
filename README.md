# atomik-ssg

A lightweight static site generator written in C.

## Why atomik-ssg?

Most static site generators are excellent tools with rich ecosystems — if you need themes, plugins, and extensive configuration, you can check out [Zola](https://www.getzola.org) or [Hugo](https://gohugo.io).

atomik-ssg is for those who want something smaller. A single binary, no runtime dependencies, no configuration overhead. Write Markdown, run build, get HTML.

## Comparison

| | atomik-ssg | Hugo | Zola |
|---|---|---|---|
| Language | C | Go | Rust |
| Binary size | ~230KB | ~8MB | ~12MB |
| Dependencies | none | none | none |
| Themes | 3 built-in | 300+ | 100+ |
| Shortcodes | ❌ | ✅ | ✅ |
| i18n | ❌ | ✅ | ✅ |
| Asset pipeline | ❌ | ✅ | ✅ |
| Learning curve | Low | Medium | Low |
| Config format | TOML | TOML/YAML | TOML |

## Features

- Markdown to HTML via [cmark](https://github.com/commonmark/cmark), linked statically
- Frontmatter support (title, date, slug, description, draft)
- Posts and standalone pages, with an automatic page menu
- Drafts, kept out of the build until you want them
- Previous/next navigation between posts
- Theme system (default, dark, sepia — or bring your own)
- TOML configuration
- Built-in development server
- Auto-generated index page, newest post first
- RSS feed
- Image support

## Installation

Download the binary for your platform from [Releases](https://github.com/yavuzselimsahin/atomik-ssg/releases) and place it in your PATH.

Or build from source:

```bash
git clone https://github.com/yavuzselimsahin/atomik-ssg.git
cd atomik-ssg
make
```

**Dependencies:** none. A C compiler and `make` are all you need — cmark is
bundled in [vendor/cmark/](vendor/cmark/) and compiled into the binary, so the
result links against nothing but the system C library.

To build against a cmark you already have installed instead:

```bash
make USE_SYSTEM_CMARK=1
```

Run the test suite with:

```bash
make test
```

## Usage

```bash
# Create a new project — asks which starter pages you want
# (about, projects, contact, uses, now) and scaffolds them
atomik-ssg init

# Create a new post
atomik-ssg new "My First Post"

# Build the site
atomik-ssg build

# Start development server (default: port 4545)
atomik-ssg serve
atomik-ssg serve 8080
```

## Project Structure

```
my-blog/
├── config.toml
├── content/
│   ├── about.md                        → /about/
│   └── posts/
│       └── 2026-08-10-my-first-post.md → /posts/my-first-post/
├── themes/
│   ├── default/
│   ├── dark/
│   └── sepia/
└── public/
```

Markdown under `content/posts/` becomes a dated post: it is listed on the index
and in the feed. Markdown directly under `content/` becomes a standalone page at
`/<slug>/` — that is where an About or Projects page goes.

Pages are linked automatically. The menu is derived from what is in `content/`,
so adding a file is all it takes; there is no list to keep in sync. It is
available to every template as `{{pages}}`.

Menu entries are sorted alphabetically by title. Set `order:` on the ones whose
position matters — those come first, in ascending order, and everything else
follows alphabetically:

```markdown
---
title: About
order: 1
---
```

Sorting transliterates before comparing, so `İletişim` lands between `About` and
`Projects` rather than after both, and the result is the same on every machine.

`atomik-ssg init` offers a set of starter pages and writes the ones you pick,
numbering their `order:` in the sequence you listed them — so the menu comes out
the way you asked for it. The About page is prefilled with the author name and
site description you already typed; the rest are outlines to edit.

## Configuration

```toml
title       = "My Blog"
description = "About my work"
base_url    = "https://example.com"
author      = "Your Name"
theme       = "default"

[build]
output_dir = "public"

[server]
port = 4545

[deploy]
host = "user@vps"
path = "/var/www/myblog"
```

`slug` and `date` are optional in a post: if either is missing it is derived
from the file name (`2026-08-10-my-first-post.md`). Posts are listed newest
first on the index and in the feed.

## Post Format

```markdown
---
title: My First Post
date: 2026-08-10
slug: my-first-post
description: A short description
draft: false
---

Content goes here.
```

Every field is optional. A missing `slug` or `date` is taken from the file name
(`2026-08-10-my-first-post.md`), and a missing `title` falls back to the slug.

Set `draft: true` to keep an entry out of the build entirely — no page, no index
entry, no feed item. Build with `--drafts` to preview them:

```bash
atomik-ssg build --drafts
```

## Template Variables

| Variable | Available in | Notes |
|---|---|---|
| `{{title}}` `{{date}}` `{{description}}` `{{slug}}` | post, page | HTML-escaped |
| `{{content}}` | post, page | rendered markdown |
| `{{site_title}}` `{{site_description}}` | all | from `config.toml` |
| `{{post_items}}` | index | the generated post list |
| `{{pages}}` | all | the page menu, as `<li>` items |
| `{{prev_url}}` `{{prev_title}}` | post | the older post, empty on the last one |
| `{{next_url}}` `{{next_title}}` | post | the newer post, empty on the first one |

Unknown placeholders are left in the output untouched. There are no conditionals:
on the first and last post the navigation variables expand to nothing, and the
bundled themes hide the empty links with `.post-nav a:empty { display: none; }`.

Themes may provide `templates/page.html`; if they do not, pages fall back to
`templates/post.html`.

## Who is this for?

atomik-ssg works well if you want a simple personal blog or documentation site, prefer editing files over using a CMS, and want a small self-contained binary.

If you need i18n, shortcodes, asset pipelines, or a large theme library, you will be better served by Hugo or Zola.

## Documentation

Full documentation is coming soon.

## License

MIT
