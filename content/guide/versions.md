---
title: Versions
description: Documenting more than one release
order: 7
---

## Showing which release this is

```toml
version = "0.3.0"
```

The docs theme shows it beside the site name. Leave the key out and nothing is
displayed, so a site that does not track releases carries no empty badge.

It is available to any template as `{{version}}`.

## Keeping older versions online

A version of the documentation is a separate build in a separate directory.
Nothing in the generator manages versions, because two things already do: git
stores the history, and [`base_path`](/deploying/) lets a site live in a
subdirectory.

```bash
git checkout v0.2.0
atomik-ssg build      # version = "0.2.0", base_path = "/v0.2.0",
                      # output_dir = "site/v0.2.0"

git checkout main
atomik-ssg build      # version = "0.3.0", base_path = "",
                      # output_dir = "site"
```

The result is the current documentation at the root and each older release
under its own path:

```
site/
├── index.html          ← 0.3.0
├── guide/
└── v0.2.0/
    ├── index.html      ← 0.2.0
    └── guide/
```

## A switcher between them

The generator does not build one, because only you know which versions exist
and which are worth keeping. It is a few lines in the theme, edited when you
cut a release:

```html
<select onchange="location.href=this.value">
  <option value="/">0.3.0</option>
  <option value="/v0.2.0/">0.2.0</option>
</select>
```

> [!NOTE]
> This is where documentation tools tend to grow. Snapshot commands, banners
> warning that a page is outdated, cross-version link rewriting and canonical
> URLs are the largest source of complexity in the tools that offer them.
> Directories and a `<select>` cover the same ground for a project this size.

## Warning readers off an old page

Set `built_with`, `edit_url` and the rest per version as usual — the config is
part of the checkout, so each version carries its own. If you want a banner on
older versions, add it to that version's template when you build it, or leave
a line in its `index.md`:

```markdown
> [!WARNING]
> This documents 0.2.0. The current release is [0.3.0](/).
```
