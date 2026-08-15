---
title: Example Sites
description: Sites built with atomik-ssg
order: 6
---

Four sites, each leaning on a different part of the tool.

## This site

<https://github.com/yavuzselimsahin/atomik-ssg>

The documentation you are reading. The `docs` theme with no modifications:
sidebar from `{{page_tree}}`, reading order for previous and next, a light and
dark palette with a switch, and the version beside the site name.

It is also the plainest demonstration of the idea, since a documentation site
is the hardest thing this generator is asked to do. [Colophon](/colophon/)
goes through it in detail, including what it cannot do.

## atom-builder

<https://yavuzselimsahin.github.io/atom-builder>

Documentation for a tool that cross-compiles native projects from one machine.
A project page rather than a domain root, so it sets both halves of the
subdirectory problem:

```toml
base_url  = "https://yavuzselimsahin.github.io/atom-builder"
base_path = "/atom-builder"
```

Without `base_path` the pages would load and every link and stylesheet would
404 against the domain root.

## Shared Clipboard

<https://yavuzselimsahin.github.io/shared-clipboard/manual/>

A manual for a desktop utility that syncs clipboards over a local network.
This one is the most interesting deployment of the four, because the
repository already served a hand-written landing page at
`/shared-clipboard/`. GitHub Pages allows one source per repository, so the
manual is built *beside* the landing page rather than over it:

```toml
base_path  = "/shared-clipboard/manual"

[build]
output_dir = "docs/manual"
```

The landing page keeps the root and links down into the manual.

Its [installation page](https://yavuzselimsahin.github.io/shared-clipboard/manual/installation/)
is worth a look for a different reason: the download list highlights the
reader's operating system, written as plain HTML, CSS and a few lines of
JavaScript inside the markdown file. The generator knows nothing about
operating systems, and with JavaScript off the reader still sees every
platform.

## yavuzselimsahin.com

<https://yavuzselimsahin.com>

A personal site on a custom domain — the blog half of the tool rather than the
documentation half. Dated posts under `content/posts/`, a chronological index,
an RSS feed, and pages such as About sitting alongside them.

Being at a domain root, it needs no `base_path` at all.

---

> [!TIP]
> Building something with it? Open an issue on
> [the repository](https://github.com/yavuzselimsahin/atomik-ssg) and it can
> go on this page.
