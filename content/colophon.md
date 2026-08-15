---
title: Colophon
description: How this site is made
order: 7
---

This site is built with atomik-ssg.

Not "powered by" in the badge sense — it is a plain atomik-ssg project. The
pages you are reading are markdown files in a `content/` directory, arranged
into the sections you see in the sidebar. It was created with `atomik-ssg
init` using the bundled `docs` theme, and it is rebuilt with `atomik-ssg
build`.

Everything in the navigation is a consequence of the file layout:

```
content/
├── installation.md
├── quick-start.md
├── guide/
│   ├── index.md
│   ├── content-model.md
│   ├── frontmatter.md
│   ├── navigation.md
│   ├── markdown.md
│   ├── drafts.md
│   ├── themes.md
│   └── versions.md
├── reference/
│   ├── index.md
│   ├── commands.md
│   ├── configuration.md
│   └── template-variables.md
├── deploying.md
└── colophon.md
```

The sidebar is generated from that tree, the previous/next links at the foot
of each page follow it top to bottom, and the order of the sections comes from
an `order:` field in each file's frontmatter. No list of pages is maintained
anywhere.

## What else is on display

- The version badge beside the site name is `version` in `config.toml`.
- The light/dark switch in the top bar is part of the docs theme.
- The boxes on [Markdown Support](/guide/markdown/) and elsewhere are
  callouts, written as `> [!NOTE]` in the markdown.
- The line at the foot of the sidebar is `built_with = true`. Commenting that
  one line out removes it.

## What it demonstrates, and what it admits

Documentation is a fair test of a static site generator, because it needs what
a blog does not: a hierarchy, a reading order, a sidebar that stays correct as
pages come and go. This site is the tool doing the job it claims to do.

It shows the limits just as plainly. The table on
[Markdown Support](/guide/markdown/) is written as raw HTML, because the
bundled parser is strict CommonMark and does not do pipe tables. Code samples
are not syntax highlighted, because no highlighter ships in the binary. Both
are real constraints of this version, visible here rather than hidden.

## The numbers

- The generator is a single binary of roughly 285 KB with no runtime
  dependencies.
- Building this site takes a few milliseconds.
- At 5000 pages a full rebuild takes about a second, most of it spent writing
  files rather than parsing markdown.

## Source

atomik-ssg is written in C and lives at
[github.com/yavuzselimsahin/atomik-ssg](https://github.com/yavuzselimsahin/atomik-ssg),
under the MIT licence.
