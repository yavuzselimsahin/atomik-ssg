#include "toml.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void trim(char *s) {
    char *start = s;
    while (*start == ' ' || *start == '\t') start++;

    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' ||
                           *end == '\r' || *end == '\n'))
        end--;
    *(end + 1) = '\0';

    if (start != s) memmove(s, start, strlen(start) + 1);
}

static void strip_quotes(char *s) {
    size_t len = strlen(s);
    if (len >= 2 && s[0] == '"' && s[len-1] == '"') {
        s[len-1] = '\0';
        memmove(s, s+1, len-1);
    }
}

int toml_parse(const char *path, TomlDoc *doc) {
    doc->count = 0;

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[640];
    char section[TOML_MAX_KEY] = "";

    while (fgets(line, sizeof(line), f)) {
        trim(line);

        /* Skip empty lines and comments */
        if (line[0] == '\0' || line[0] == '#') continue;

        /* Section header: [build] */
        if (line[0] == '[') {
            char *end = strchr(line, ']');
            if (!end) continue;
            *end = '\0';
            strncpy(section, line + 1, TOML_MAX_KEY - 1);
            trim(section);
            continue;
        }

        /* key = value */
        char *eq = strchr(line, '=');
        if (!eq) continue;

        if (doc->count >= TOML_MAX_PAIRS) break;

        TomlPair *pair = &doc->pairs[doc->count++];

        *eq = '\0';
        strncpy(pair->key, line, TOML_MAX_KEY - 1);
        trim(pair->key);

        strncpy(pair->val, eq + 1, TOML_MAX_VAL - 1);
        trim(pair->val);
        strip_quotes(pair->val);

        strncpy(pair->section, section, TOML_MAX_KEY - 1);
    }

    fclose(f);
    return 0;
}

const char *toml_get(const TomlDoc *doc, const char *section, const char *key) {
    for (int i = 0; i < doc->count; i++) {
        const TomlPair *p = &doc->pairs[i];
        if (strcmp(p->key, key) == 0 &&
            strcmp(p->section, section) == 0)
            return p->val;
    }
    return NULL;
}

const char *toml_get_or(const TomlDoc *doc, const char *section,
                        const char *key, const char *fallback) {
    const char *val = toml_get(doc, section, key);
    return val ? val : fallback;
}