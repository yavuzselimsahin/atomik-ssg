---
title: Content Model
description: How files become URLs
order: 1
---

atomik-ssg has exactly two kinds of content, and which one you get depends only
on where the file sits.

## Posts

Markdown under `content/posts/` is a **post**. Posts are dated, listed newest
first on the index page, and carry an entry in the RSS feed.

```
content/posts/2026-08-10-hello-world.md   →   /posts/hello-world/
```

## Pages

Markdown anywhere else under `content/` is a **page**. Pages are not dated, do
not appear in the feed, and are linked from the site menu instead.

```
content/about.md                →   /about/
content/guide/navigation.md     →   /guide/navigation/
content/guide/index.md          →   /guide/
```

That is the whole distinction. There is no front-matter switch and no list of
routes: the location of the file *is* the URL.

## Sections

Directories nest as deeply as you like, and each one becomes a section.

Giving a directory an `index.md` makes the section itself a page — that is how
`/guide/` above became a real URL rather than just a heading. Leave it out and
the section is a heading over its children, which is useful when you want to
group pages without writing a landing page for the group.

## Slugs

The URL segment comes from the file name: lowercased, with anything that is not
a letter, digit or dash collapsed into a single dash. Accented and non-Latin
letters are transliterated rather than dropped, so

```
content/Yapılandırma.md   →   /yapilandirma/
content/Çöp Kutusu.md     →   /cop-kutusu/
```

A post's file name may carry a `YYYY-MM-DD-` prefix; it is used as the date and
stripped from the slug.

Set `slug:` in the frontmatter to override the last segment:

```markdown
---
title: Installation Instructions
slug: install
---
```

Slugs are always normalised, whether they came from a file name or from
frontmatter, so a slug can never contain a path separator and can never escape
the output directory.

## Output

A build writes one directory per entry, each holding an `index.html`:

```
public/
├── index.html
├── rss.xml
├── about/index.html
├── guide/index.html
├── guide/navigation/index.html
└── posts/hello-world/index.html
```

This is why every URL ends in a slash. The development server redirects
`/about` to `/about/` for you; most static hosts do the same.
