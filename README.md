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
- Posts, plus pages that nest into sections for documentation
- Automatic page menu and sidebar, derived from `content/`
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
│   ├── guide/
│   │   ├── index.md                    → /guide/
│   │   └── install.md                  → /guide/install/
│   └── posts/
│       └── 2026-08-10-my-first-post.md → /posts/my-first-post/
├── themes/
│   ├── default/
│   ├── dark/
│   └── sepia/
└── public/
```

Directories nest as deeply as you like, and a directory's `index.md` becomes the
directory itself rather than a child of it. `content/posts/` is the one
exception: it holds the dated posts and stays out of the page hierarchy.

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

## Documentation Sites

The same machinery builds a documentation site: nest your pages into sections
and give them an `order:`, and the tool derives a sidebar and a reading order
from the result.

`{{pages}}` carries the top level only, which suits a menu bar. `{{page_tree}}`
carries the whole hierarchy as a nested `<ul>`, which suits a sidebar:

```html
<aside>{{page_tree}}</aside>
```

A section with an `index.md` becomes a link; one without becomes a heading, so
you can group pages without inventing a landing page for the group.

`{{prev_*}}` and `{{next_*}}` follow that sidebar top to bottom — "Next:
Configuration" means the next thing to read. Posts work the same way against
the index listing: `next` is the entry below the current one, which on a
newest-first index means the older post. The arrows always agree with the
direction the list runs.

Set `edit_url` to offer an edit link on every page:

```toml
edit_url = "https://github.com/you/project/edit/main"
```

`{{edit_url}}` then expands to that base joined with the page's source file, and
`{{source_path}}` gives the source file on its own. Both are empty when
`edit_url` is unset, so a theme can carry the link unconditionally.

Highlighting a code block is left to the theme: cmark already emits
`<code class="language-c">`, so dropping a highlighter into the theme's
`static/` is enough and the binary stays dependency-free.

### The docs theme

`atomik-ssg init` offers a fourth theme, `docs`, built around this: a sticky
sidebar carrying `{{page_tree}}`, a top bar with the section menu, prev/next
cards at the foot of every page, an edit link that disappears when `edit_url`
is unset, and a palette that follows the reader's system light/dark setting.

Choosing it changes what `init` scaffolds. A normal site is asked which
starter pages it wants (About, Projects, Contact…) and gets a sample post; a
docs site is asked nothing and gets a short guide to writing documentation
with atomik-ssg instead — Getting Started, a Writing section and a Reference
section, nested and ordered. It is a worked example of everything above, and
it is yours to edit or delete.

Its landing page is a table of contents rather than a feed. `content/posts/`
is optional for a docs site: leave it empty, or delete it.

The sidebar is generated once for the whole site, so the one thing it cannot
know is which page it ended up on. Five lines of JavaScript at the end of the
template mark the current entry — no per-page rebuild, no template
conditionals.

### Keeping several versions

A version of the documentation is just a separate build in a separate
directory, which is what `base_path` already makes possible. Nothing in the
generator manages versions, because git already stores them:

```bash
git checkout v1.0.0
atomik-ssg build            # base_path = "/v1.0.0", output_dir = "site/v1.0.0"
git checkout main
atomik-ssg build            # base_path = "",        output_dir = "site"
```

Set `version` in each one and the theme shows which release the reader is on.
A switcher between versions is a few lines of markup in the theme, added when
you cut a release — deliberately not a feature of the binary, because version
management is where documentation tools get big.

### Markdown support

The bundled parser is [cmark](https://github.com/commonmark/cmark), which
implements CommonMark exactly. Raw HTML in your markdown is passed through, so
anything the syntax does not cover can be written by hand.

GitHub's extensions are **not** part of CommonMark and are therefore not
available: pipe tables, `~~strikethrough~~`, bare-URL autolinks and task lists
all render as literal text. Write a table as HTML if you need one.

### Callouts

GitHub's alert syntax is recognised, which CommonMark leaves as an ordinary
quote and GitHub itself handles outside the parser:

```markdown
> [!WARNING]
> `--delete` removes remote files that are not in your output.
```

`NOTE`, `TIP`, `IMPORTANT`, `WARNING` and `CAUTION` become
`<blockquote class="callout callout-warning">`, with the marker dropped from
the text. Anything else stays an ordinary quote, so a typo is visible rather
than swallowed.

The label is drawn by the theme's CSS rather than written into the HTML, so
renaming or translating it is a stylesheet edit:

```css
.callout-warning::before { content: "Dikkat"; }
```

## Configuration

```toml
title       = "My Blog"
description = "About my work"
base_url    = "https://example.com"
author      = "Your Name"
theme       = "default"
built_with  = true

[build]
output_dir = "public"       # or a nested path such as "docs/manual"

[server]
port = 4545

[deploy]
host = "user@vps"
path = "/var/www/myblog"
```

A few keys are worth calling out:

- **`edit_url`** — a base URL that a page's source path is appended to, which
  is how a theme offers "Edit this page". Unset, and the themes hide the link
  rather than showing a dead one.
- **`version`** — the release this documentation describes. The docs theme
  shows it beside the site name; unset, nothing is displayed.
- **`base_path`** — the subdirectory the finished site is served from, such
  as `/my-project` on GitHub project pages. Every generated link is prefixed
  with it, including the root-absolute links you write in markdown, and
  templates get it as `{{base_path}}`. Unset means the site lives at a domain
  root. Set `base_url` to the full address including the same path, so the
  feed's absolute links match.
- **`built_with`** — puts a quiet "Generated with atomik-ssg" line in the
  footer of every page, linked to the project. `init` writes it enabled;
  comment the line out to remove it. An absent key counts as off.
- **`[build] output_dir`** — where the site is written, relative to the
  project. Defaults to `public`, and may be nested: `docs/manual` is what a
  project page needs when a landing page already occupies the root. Missing
  levels are created for you; absolute paths and `..` segments are refused.

Remember that a top-level key has to appear *before* the first `[section]`
header, or it lands inside whichever section came last.

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
| `{{pages}}` | all | the top-level page menu, as `<li>` items |
| `{{page_tree}}` | all | the whole page hierarchy, as a nested `<ul>` |
| `{{source_path}}` | post, page | the markdown file it was built from |
| `{{edit_url}}` | post, page | `edit_url` joined with the source path |
| `{{built_with}}` | all | the attribution link, or empty when `built_with` is off |
| `{{base_path}}` | all | `base_path` from the config; empty at a domain root |
| `{{version}}` | all | `version` from the config; empty when unset |
| `{{prev_url}}` `{{prev_title}}` | post, page | previous in reading order, empty at the start |
| `{{next_url}}` `{{next_title}}` | post, page | next in reading order, empty at the end |

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
