---
title: Frontmatter
description: The fields a page or post can set
order: 2
---

Every markdown file starts with a block fenced by `---`:

```markdown
---
title: My First Post
date: 2026-08-10
slug: my-first-post
description: A short summary
draft: false
---

The content starts here.
```

The block is required — a file without one is skipped, with a warning naming
the file. That is deliberate: it keeps stray `NOTES.md` and `TODO.md` files out
of your published site.

Everything *inside* the block is optional.

## Fields

- **`title`** — shown as the page heading and in the menu and sidebar. Falls
  back to the slug when omitted.
- **`description`** — used for the page's `<meta name="description">` and, on
  posts, as the summary in the index listing and the feed.
- **`slug`** — overrides the URL segment otherwise taken from the file name.
- **`date`** — posts only, written as `YYYY-MM-DD`. Taken from a
  `YYYY-MM-DD-` file name prefix when omitted. Posts are ordered by it, newest
  first, and it becomes the feed's publication date.
- **`draft`** — `true` keeps the entry out of the build entirely. See
  [Drafts](/guide/drafts/).
- **`order`** — position in the menu and sidebar, lowest first. Entries
  without one follow alphabetically. See [Navigation](/guide/navigation/).

Unknown keys are ignored, so you can keep notes of your own in there.

## Values

Values are read literally to the end of the line. There is no quoting, no
escaping and no multi-line syntax, so a title containing a colon is fine:

```markdown
title: C: A Modern Approach
```

Anything you write is HTML-escaped on the way out, so an ampersand or an angle
bracket in a title is safe.
