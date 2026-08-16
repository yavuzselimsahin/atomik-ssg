---
title: Navigation
description: Menus, sidebars and reading order
order: 3
---

Navigation is derived from what is in `content/`. Adding a file is enough to
publish and link it; there is no menu to keep in sync, and nothing to forget
when you rename something.

## Ordering

Entries without an `order:` are sorted alphabetically by title. Add `order:`
to the ones whose position matters and they move to the front, lowest number
first:

```markdown
---
title: Getting Started
order: 1
---
```

Ordering applies independently at each level, so a section's children are
arranged among themselves.

> [!NOTE]
> Titles are compared after transliteration rather than byte by byte. Without
> that, every title beginning with a non-ASCII letter would sort below every
> ASCII one — `İletişim` would land after `Projects` instead of between
> `About` and it. It also means the order is identical on every machine,
> which relying on the system locale would not be.

## Two shapes of navigation

`{{pages}}` gives the **top level only**, as `<li>` items. That suits a menu
bar: it stays short however deep the tree grows.

`{{page_tree}}` gives the **whole hierarchy** as a nested `<ul>`. That suits a
sidebar. Sections with an `index.md` render as links; sections without one
render as a `<span>`, so you get a heading rather than a dead link.

Both are available to every template, so a post can carry the same navigation
as a page.

## Previous and next

Every entry carries `{{prev_url}}`, `{{prev_title}}`, `{{next_url}}` and
`{{next_title}}`.

They are **positional**, in both cases: `prev` is the entry above the current
one in the list a reader was just looking at, `next` is the entry below it.

On a **page** that list is the sidebar. A section leads into its first child,
and the last child of a section leads into the next section. Reorder the
sidebar and they reorder with it.

On a **post** it is the index listing. Since the index runs newest first,
`next` is the older post — the one further down the page the reader clicked
from. Wiring these to the calendar instead would make "next" point back up
the list, which is the opposite of what an arrow pointing forward suggests.

At the two ends of the sequence the variables expand to nothing. There are no
conditionals in the template language, and for this it needs none — the
bundled themes hide the empty links with one rule:

```css
.post-nav a:empty { display: none; }
```
