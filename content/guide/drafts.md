---
title: Drafts
description: Work in progress that stays unpublished
order: 4
---

Set `draft: true` on any post or page:

```markdown
---
title: Something unfinished
draft: true
---
```

A draft is excluded from the build completely — no HTML file, no menu or
sidebar entry, no index listing, no feed item. It cannot be reached by guessing
the URL, because the page was never written.

The build tells you how many it skipped, so a draft cannot quietly vanish:

```
Build complete: 12 post(s), 4 page(s), 2 draft(s) skipped -> public/
Run with --drafts to include them.
```

## Previewing drafts

```bash
atomik-ssg build --drafts
```

This builds everything, drafts included, so you can read them locally. Just
remember the output directory now contains them: build again without the flag
before you deploy.

`--drafts` applies to `build` and `deploy`. Passing it to any other command is
an error rather than a silent no-op.

## Accepted spellings

`true`, `yes`, `on` and `1` all count as true, in any capitalisation. Anything
else — including the absence of the field — means false.
