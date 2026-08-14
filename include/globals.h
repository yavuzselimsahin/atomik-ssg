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

/* config.toml `edit_url`: the base a source path is appended to, so a theme
   can offer "edit this page". Empty when unset. */
extern char g_edit_url[512];

#endif
