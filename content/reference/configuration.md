---
title: Configuration
description: Every key in config.toml
order: 2
---

`config.toml` sits at the root of a project. Every key is optional.

```toml
title       = "My Site"
description = "What it is about"
base_url    = "https://example.com"
author      = "Your Name"
theme       = "docs"
version     = "1.0.0"
edit_url    = "https://github.com/you/project/edit/main"
base_path   = "/project"
built_with  = true

[build]
output_dir = "public"

[server]
port = 4545

[deploy]
host = "user@vps"
path = "/var/www/mysite"
```

## Identity

- **`title`** — the site name. Available to templates as `{{site_title}}`, and
  used as the feed title.
- **`description`** — a one-line summary. Available as `{{site_description}}`.
- **`author`** — recorded for your own use; `init` also uses it to prefill an
  About page.
- **`version`** — the release this documentation describes. The docs theme
  prints it next to the site name. Unset, nothing appears.

## Addresses

- **`base_url`** — the public address of the finished site, used to build the
  absolute links in the RSS feed. A trailing slash is trimmed for you.
- **`base_path`** — the subdirectory the site is served from, such as
  `/my-project` on GitHub project pages. Every link the generator writes is
  prefixed with it, and so are the root-absolute links you write in markdown,
  so `[Guide](/guide/)` keeps working. Templates get it as `{{base_path}}`,
  which is how the bundled themes reach the stylesheet. Leave it unset for a
  domain root, and include the same path in `base_url`.

> [!WARNING]
> A top-level key has to appear **before** the first `[section]` header.
> Appending `base_path` to the end of the file would place it inside whichever
> section came last, where nothing is looking for it.

## Appearance

- **`theme`** — the directory name under `themes/`. A value containing a path
  separator is rejected and the default is used instead.
- **`edit_url`** — a base URL that a page's source path is appended to, which
  is how the docs theme offers "Edit this page". Unset, and the themes hide
  the link rather than showing a dead one.
- **`built_with`** — `true` adds a quiet line noting that the site was
  generated with atomik-ssg, linked to the project. `init` writes it enabled;
  comment the line out to remove it. An absent key counts as off, so nothing
  is added to a site that never asked for it.

## `[build]`

- **`output_dir`** — where the site is written. Defaults to `public`. Like
  `theme`, it must be a plain directory name.

## `[server]`

- **`port`** — the port `serve` uses when none is given on the command line.
  Defaults to 4545.

## `[deploy]`

- **`host`** — an rsync or SSH destination, such as `user@example.com`.
- **`path`** — the directory on that host to mirror into.

Both values are quoted before they reach the shell, so a stray character in
them cannot turn into a command.

## Notes on the format

The parser handles the subset of TOML this tool needs: comments beginning with
`#`, `key = value` pairs, quoted or bare values, and `[section]` headers.
