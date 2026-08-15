---
title: Markdown Support
description: What the parser does and does not do
order: 6
---

Markdown is rendered by [cmark](https://github.com/commonmark/cmark), the
reference implementation of [CommonMark](https://commonmark.org). It is
compiled into the binary, so there is nothing to install and no version skew
between machines.

## What works

Everything in the CommonMark specification: headings, emphasis, lists, links,
images, blockquotes, fenced and indented code blocks, thematic breaks,
reference links, HTML blocks, hard line breaks, and entity references.

## Raw HTML

Raw HTML in your markdown is passed straight through:

```markdown
Some text.

<figure>
  <img src="/images/diagram.png" alt="">
  <figcaption>A diagram</figcaption>
</figure>
```

cmark suppresses raw HTML by default, as a guard against markdown submitted by
strangers. Here the markdown is your own, and silently deleting your `<iframe>`
would be the surprising behaviour, so it is enabled.

## What does not work

GitHub's extensions are **not** part of CommonMark and are not available:

- pipe tables
- `~~strikethrough~~`
- bare-URL autolinks — write `<https://example.com>` or a normal link
- task list checkboxes
- footnotes

These render as literal text rather than failing loudly, which is worth
knowing before you paste in something written for GitHub.

## Callouts

GitHub's alert syntax is recognised, even though CommonMark treats it as an
ordinary quote:

```markdown
> [!WARNING]
> This removes files on the far end.
```

> [!WARNING]
> That block produced this box.

Five kinds are available: `NOTE`, `TIP`, `IMPORTANT`, `WARNING` and `CAUTION`.
Anything else is left as a plain quote with the marker intact, so a
misspelling shows up instead of disappearing.

> [!TIP]
> The label comes from the theme's CSS, not the HTML, so changing or
> translating it is one line in the stylesheet.

## Tables, in the meantime

Because raw HTML passes through, a table can be written directly:

<table>
  <tr><th>Command</th><th>Does</th></tr>
  <tr><td><code>build</code></td><td>Generates the site</td></tr>
  <tr><td><code>serve</code></td><td>Previews it locally</td></tr>
</table>

That table is HTML inside a markdown file, on this page, in this site. It is
more typing than a pipe table, and it is the honest workaround until the parser
gains table support.
