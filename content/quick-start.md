---
title: Quick Start
description: A working site in about two minutes
order: 2
---

## Create a project

```bash
atomik-ssg init
```

`init` asks a handful of questions — the project name, the site title, an
author, a base URL — and then which theme you want. The theme decides what
kind of site it scaffolds:

- **default**, **dark** and **sepia** build a blog or a personal site. You are
  asked which starter pages you want (About, Projects, Contact, Uses, Now) and
  you get a sample post to look at.
- **docs** builds a documentation site. You are asked nothing further and get a
  short guide to writing documentation, already nested into sections.

Every answer has a default, so pressing Enter through the whole thing is a
perfectly good way to start.

## Build it

```bash
cd my-site
atomik-ssg build
```

This writes the finished site into `public/`. Those are plain HTML files with
no server-side anything, ready to be copied to any host.

## Preview it

```bash
atomik-ssg serve
```

Then open <http://localhost:4545>. The development server reads from `public/`,
so after changing a file run `build` again and refresh the page.

The server binds to localhost only. It serves your working directory, which has
no business being reachable from the rest of the network.

## Write something

```bash
atomik-ssg new My First Post
```

That creates a dated markdown file under `content/posts/` with the frontmatter
already filled in. Open it, write, and build again.

To add a page rather than a post, create a markdown file directly under
`content/`:

```markdown
---
title: About
---

A few words about me.
```

Build, and it is published at `/about/` and linked from the menu. Nothing needs
registering anywhere.
