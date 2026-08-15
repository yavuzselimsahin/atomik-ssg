---
title: Commands
description: The command line in full
order: 1
---

## `atomik-ssg init`

Creates a project in a new directory, asking for a name, title, description,
author, base URL, deploy target and theme. Which theme you choose decides
whether you are asked about starter pages or handed a documentation scaffold.

Refuses to touch a directory that already exists, and rejects a project name
containing a path separator.

## `atomik-ssg build`

Reads `content/`, writes the site into `public/` (or whatever `output_dir`
says). Reports how many posts, pages and drafts it handled.

`--drafts` includes entries marked `draft: true`.

## `atomik-ssg new <title>`

Creates a dated post under `content/posts/` with frontmatter filled in. Quotes
are optional — `atomik-ssg new My First Post` works, and so does the quoted
form.

The file name is today's date plus a slug derived from the title.

## `atomik-ssg serve [port]`

Serves the output directory over HTTP on localhost. Without an argument it uses
`[server] port` from `config.toml`, falling back to 4545.

It is a preview server, not a production one: it binds to the loopback
interface only, refuses paths that try to climb out of the output directory,
redirects a directory to its trailing-slash form, and serves nothing it was not
asked for.

It does not rebuild on change. Run `build` again and refresh.

## `atomik-ssg deploy`

Builds, then mirrors the output to the host in `[deploy]` using `rsync -avz
--delete`. Because `--delete` removes remote files that are not in your output,
it asks for confirmation first and prints the exact command it will run.

`--drafts` is accepted here too, and applies to the build.

## `atomik-ssg help`

Prints a summary of all of the above.
