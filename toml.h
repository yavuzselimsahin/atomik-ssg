#ifndef TOML_H
#define TOML_H

#define TOML_MAX_KEY   128
#define TOML_MAX_VAL   512
#define TOML_MAX_PAIRS 64

typedef struct {
    char key[TOML_MAX_KEY];
    char val[TOML_MAX_VAL];
    char section[TOML_MAX_KEY];
} TomlPair;

typedef struct {
    TomlPair pairs[TOML_MAX_PAIRS];
    int count;
} TomlDoc;

int toml_parse(const char *path, TomlDoc *doc);
const char *toml_get(const TomlDoc *doc, const char *section, const char *key);
const char *toml_get_or(const TomlDoc *doc, const char *section,
                        const char *key, const char *fallback);

#endif