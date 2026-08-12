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
- Frontmatter support (title, date, slug, description)
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
# Create a new project
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
│   └── posts/
│       └── 2026-08-10-my-first-post.md
├── themes/
│   ├── default/
│   ├── dark/
│   └── sepia/
└── public/
```

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
---

Content goes here.
```

## Who is this for?

atomik-ssg works well if you want a simple personal blog or documentation site, prefer editing files over using a CMS, and want a small self-contained binary.

If you need i18n, shortcodes, asset pipelines, or a large theme library, you will be better served by Hugo or Zola.

## Documentation

Full documentation is coming soon.

## License

MIT
