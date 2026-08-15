---
title: Deploying
description: Getting the output onto a server
order: 5
---

A build produces plain files in `public/`. Anything that can serve a directory
can serve them: a VPS, GitHub Pages, Netlify, Cloudflare Pages, S3, or a
Raspberry Pi in a cupboard.

## With the built-in deploy

Set the target in `config.toml`:

```toml
[deploy]
host = "user@example.com"
path = "/var/www/mysite"
```

Then:

```bash
atomik-ssg deploy
```

This builds first, shows you the exact `rsync` command, and asks before running
it. The confirmation is there because the command uses `--delete`, which
removes files on the far end that are not in your output — exactly what you
want for a mirror, and exactly what you want to be sure about first.

Authentication is whatever your SSH configuration already does; atomik-ssg
neither asks for nor stores credentials.

## By hand

`deploy` is a convenience, not a requirement. There is nothing special about
the output:

```bash
atomik-ssg build
rsync -avz --delete public/ user@example.com:/var/www/mysite/
```

## On GitHub Pages

A user or organisation site (`you.github.io`) is served from a domain root, so
it needs nothing special.

A project site is served from a subdirectory (`you.github.io/project/`), so
tell the generator where it will live:

```toml
base_path = "/project"
base_url  = "https://you.github.io/project"
```

Every link is then written with that prefix. Without it the pages load but the
stylesheet and every internal link resolve against the domain root and 404.

If the repository already serves something at that root — a hand-written
landing page, say — build the site into a subdirectory beside it rather than
over it:

```toml
base_path  = "/project/manual"
base_url   = "https://you.github.io/project/manual"

[build]
output_dir = "docs/manual"
```

`output_dir` may be nested, and the missing levels are created for you. The
landing page keeps `/project/` and the generated site takes `/project/manual/`.
GitHub Pages serves one source per repository, so this is how both live at
once.

## In CI

Because the binary has no runtime dependencies, a CI job needs no toolchain
beyond a C compiler:

```yaml
- run: make
- run: ./atomik-ssg build
- run: rsync -avz --delete public/ "$TARGET"
```

## Before you go live

- Set `base_url` to the real address. The RSS feed builds absolute links from
  it, and a placeholder there produces a feed pointing at `localhost`.
- Build without `--drafts`, so nothing unfinished ships.
- Point your host at `public/`, not at the project root — the project root also
  contains your sources.
