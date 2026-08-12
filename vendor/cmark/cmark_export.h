/* Hand-written replacement for the header CMake's generate_export_header()
   produces. atomik-ssg links cmark statically, so every export macro is empty.
   Kept byte-compatible in spirit with the upstream 0.31.2 output. */
#ifndef CMARK_EXPORT_H
#define CMARK_EXPORT_H

#ifndef CMARK_EXPORT
#  define CMARK_EXPORT
#endif

#ifndef CMARK_NO_EXPORT
#  define CMARK_NO_EXPORT
#endif

#ifndef CMARK_DEPRECATED
#  define CMARK_DEPRECATED __attribute__((__deprecated__))
#endif

#ifndef CMARK_DEPRECATED_EXPORT
#  define CMARK_DEPRECATED_EXPORT CMARK_EXPORT CMARK_DEPRECATED
#endif

#ifndef CMARK_DEPRECATED_NO_EXPORT
#  define CMARK_DEPRECATED_NO_EXPORT CMARK_NO_EXPORT CMARK_DEPRECATED
#endif

#endif /* CMARK_EXPORT_H */
