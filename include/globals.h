#ifndef GLOBALS_H
#define GLOBALS_H

#include "../toml.h"
#include "parser.h"

extern TomlDoc g_toml;
extern char    g_theme_path[512];
extern char    g_output_dir[256];
extern char    g_site_title[MAX_FIELD];
extern char    g_site_description[MAX_FIELD];

/* Set by --drafts: include posts marked `draft: true` in the build. */
extern int     g_include_drafts;

/* The <li> items of the page menu, built once per build and available to every
   template as {{pages}}. Empty when the site has no pages. */
extern const char *g_nav_html;

/* The nested <ul> of the whole page hierarchy, available as {{page_tree}}. */
extern const char *g_tree_html;

/* config.toml `version`: shown by a theme so a reader can tell which release
   they are reading about. Empty when unset. */
extern char g_version[64];

/* config.toml `base_path`: the subdirectory the finished site is served
   from, normalised to "" or "/prefix". Every generated link is prefixed with
   it, and templates get it as {{base_path}}. */
extern char g_base_path[256];

/* config.toml `built_with`: the attribution link, ready to place, or empty
   when the key is absent or false. Commenting the key out removes it. */
extern char g_built_with[256];

/* config.toml `edit_url`: the base a source path is appended to, so a theme
   can offer "edit this page". Empty when unset. */
extern char g_edit_url[512];

#endif
