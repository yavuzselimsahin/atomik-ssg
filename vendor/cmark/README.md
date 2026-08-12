# Vendored cmark

Upstream: <https://github.com/commonmark/cmark>
Version:  0.31.2
License:  BSD-2-Clause — see [COPYING](COPYING)

These are the unmodified `src/*.c`, `src/*.h` and `src/*.inc` files from the
0.31.2 release, minus `main.c` (the `cmark` command-line tool, which
atomik-ssg does not use).

Two headers are normally produced by cmark's CMake build and are supplied here
by hand instead, so that `make` needs neither CMake nor a preinstalled cmark:

- `cmark_export.h` — CMake's `generate_export_header()` output, reduced to
  empty macros because we link statically.
- `cmark_version.h` — CMake's `configure_file()` output for
  `cmark_version.h.in`, with 0.31.2 filled in.

`scanners.c` is upstream's checked-in output of `re2c`, so re2c is not needed
either.

## Updating

1. Download the new release tarball from the upstream releases page.
2. Copy `src/*.c`, `src/*.h`, `src/*.inc` and `COPYING` over this directory.
3. Delete `main.c` and `cmark_version.h.in`.
4. Bump the version numbers in `cmark_version.h`.
5. Run `make && make test`.

To build against a system-installed cmark instead of these sources:

```bash
make USE_SYSTEM_CMARK=1
```
