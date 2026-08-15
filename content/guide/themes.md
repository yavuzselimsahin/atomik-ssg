---
title: Themes
description: The four bundled themes, and writing your own
order: 6
---

A theme is a directory under `themes/` holding `templates/` and `static/`.
`init` writes all four; `config.toml` picks which one is used:

```toml
theme = "docs"
```

## The bundled themes

- **default** — light and minimal, serif body text. A personal blog.
- **dark** — a terminal feel, monospaced.
- **sepia** — warm and book-like, for long-form reading.
- **docs** — a two-column documentation layout, described below.

The three blog themes share their templates and differ only in their
stylesheet. The docs theme has templates of its own, because a sidebar is a
structural difference rather than a cosmetic one.

## What the docs theme adds

- A sticky sidebar carrying `{{page_tree}}`. Only the tree scrolls; what sits
  beneath it stays put.
- Previous/next cards at the foot of every page, following the reading order.
- A light and a dark palette, taken from the `default` and `dark` themes so a
  site that mixes documentation and posts still looks like one thing.
- A switch between the two in the top bar. The choice is remembered, and a
  small script in `<head>` applies it before the first paint so the other
  scheme never flashes first. With no choice stored, the reader's system
  setting decides.
- The `version` from your config, shown beside the site name.
- An "Edit this page" link, which disappears when `edit_url` is unset.
- Colours for each kind of [callout](/guide/markdown/).

## Files a theme provides

- `templates/index.html` — the site's front page
- `templates/post.html` — a single post
- `templates/page.html` — a single page. Optional: pages fall back to
  `post.html` when it is missing.

Everything in `static/` is copied to the root of the output, so
`themes/docs/static/style.css` is served at `/style.css`.

A project-level `static/` directory is copied on top of that, which is where
images and anything else of your own should go.

> [!NOTE]
> Themes are copied into your project when you run `init`, so they are yours
> to edit. The flip side is that a later version of atomik-ssg will not update
> them; a fix to a bundled theme reaches an existing project only if you copy
> it across.

## Writing your own

Copy one of the bundled themes, rename the directory, point `theme` at it and
edit. The full list of placeholders is in
[Template Variables](/reference/template-variables/). Unknown placeholders are
left untouched in the output, so a theme can carry markup the generator knows
nothing about.

## Syntax highlighting

There is no highlighter in the binary, on purpose. cmark already emits the
language as a class:

```html
<pre><code class="language-c">
```

so dropping a highlighter into your theme's `static/` and referencing it from
the template is enough. The choice — and the download — stays with the theme
rather than being forced on every site.
