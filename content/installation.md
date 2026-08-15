---
title: Installation
description: Get the binary, or build it from source
order: 1
---

atomik-ssg is a single executable with no runtime dependencies. It links
against nothing but the system C library, so once you have the binary there is
nothing else to install.

## Download a binary

Grab the build for your platform from the
[releases page](https://github.com/yavuzselimsahin/atomik-ssg/releases) and put
it somewhere on your `PATH`:

```bash
chmod +x atomik-ssg
sudo mv atomik-ssg /usr/local/bin/
```

## Build from source

You need a C compiler and `make`. That is the whole list — the markdown parser
is bundled with the source and compiled into the binary.

```bash
git clone https://github.com/yavuzselimsahin/atomik-ssg.git
cd atomik-ssg
make
```

The result is a self-contained `atomik-ssg` of roughly 270 KB.

If you would rather link against a cmark you already have installed, build with
`make USE_SYSTEM_CMARK=1` instead. The bundled copy is preferred because it is
what keeps a plain `make` free of prerequisites.

Run the test suite with `make test`.

## Check it works

```bash
atomik-ssg help
```
