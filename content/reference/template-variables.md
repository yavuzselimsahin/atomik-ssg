---
title: Template Variables
description: Everything a template can place
order: 3
---

Templates live in `themes/<name>/templates/`. Any `{{name}}` from the list
below is replaced; anything unrecognised is left in the output untouched.

Values are HTML-escaped unless noted as raw.

## This entry

- **`{{title}}`** — the page or post title
- **`{{description}}`** — its description
- **`{{slug}}`** — its URL path
- **`{{date}}`** — posts only; empty on pages
- **`{{content}}`** — the rendered markdown. Raw
- **`{{source_path}}`** — the markdown file it was built from, such as
  `content/guide/navigation.md`
- **`{{edit_url}}`** — `edit_url` from the config joined to that path. Empty
  when the config does not set one

## The site

- **`{{site_title}}`** — `title` from `config.toml`
- **`{{site_description}}`** — `description` from `config.toml`
- **`{{version}}`** — `version` from `config.toml`, empty when unset
- **`{{base_path}}`** — the subdirectory the site is served from, empty at a
  domain root. Prefix your own links with it so they survive a move
- **`{{built_with}}`** — the attribution link, or empty when `built_with` is
  off. Raw

## Navigation

- **`{{pages}}`** — the top-level menu, as `<li>` items. Raw
- **`{{page_tree}}`** — the whole page hierarchy, as a nested `<ul>`. Raw
- **`{{post_items}}`** — the chronological post list, as `<li>` items. Raw.
  Index template only
- **`{{prev_url}}`**, **`{{prev_title}}`** — the previous entry in reading
  order, empty at the start
- **`{{next_url}}`**, **`{{next_title}}`** — the next entry, empty at the end

## Notes

There are no conditionals, loops or includes. The whole language is "find a
placeholder, put a value there".

Where a condition seems necessary, it usually is not. The bundled themes
handle the ends of a sequence with `a:empty { display: none }`, an unset edit
link with `a[href=""] { display: none }`, and an absent version badge the same
way — one CSS rule each, instead of a template engine.
