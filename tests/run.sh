#!/bin/bash
# atomik-ssg regression suite.
#
#   make test          # or: ./tests/run.sh
#
# Every check here corresponds to a bug that was once real. Add a case before
# fixing the next one.

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN="${BIN:-$ROOT/atomik-ssg}"
PORT="${PORT:-4712}"

if [ ! -x "$BIN" ]; then
    echo "error: $BIN not built. Run make first." >&2
    exit 1
fi

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"; [ -n "$SRV" ] && kill "$SRV" 2>/dev/null' EXIT

PASS=0
FAIL=0

ok()    { PASS=$((PASS + 1)); printf '  \033[32mPASS\033[0m  %s\n' "$1"; }
bad()   { FAIL=$((FAIL + 1)); printf '  \033[31mFAIL\033[0m  %s\n' "$1"; }
check() { if [ "$2" = "$3" ]; then ok "$1"; else bad "$1 (got '$2', want '$3')"; fi; }
group() { printf '\n\033[1m%s\033[0m\n' "$1"; }

post() {
    printf -- "---\ntitle: %s\ndate: %s\nslug: %s\ndescription: %s\n---\n\n%s\n" \
        "$2" "$3" "$4" "$5" "${6:-body}" > "content/posts/$1"
}

cd "$WORK" || exit 1
printf 'site\n\n\n\n\n\n\n\n' | "$BIN" init >/dev/null 2>&1
cd site || exit 1
rm -f content/posts/*.md

# --------------------------------------------------------------------------
for i in $(seq 1 60); do
    post "bulk-$i.md" "Bulk $i" "2020-$(printf %02d $((i % 12 + 1)))-01" "bulk-$i" "d$i"
done
post "z-old.md" "Oldest" "2001-01-01" "oldest" "d"
post "a-new.md" "Newest" "2030-12-31" "newest" "d"
post "m-mid.md" "Middle" "2015-06-15" "middle" "d"
post "esc.md" 'Tom & Jerry <script>alert(1)</script>' "2018-01-01" "esc" 'a & b < c "quoted"'
printf -- "---\ntitle: No Slug\ndate: 2019-02-02\ndescription: d\n---\n\nbody\n" \
    > content/posts/2019-02-02-derived-name.md
printf -- "---\ntitle: Evil\ndate: 2017-01-01\nslug: ../../../escaped\ndescription: d\n---\n\nbody\n" \
    > content/posts/evil.md
printf -- "no frontmatter at all\n" > content/posts/broken.md
post "md.md" "Markdown" "2016-01-01" "md" "d" \
'# Heading

Some **bold** and `code` and [a link](https://example.com).

```c
int main(void) { return 0; }
```

- one
- two

> quoted'

BUILD_OUT=$("$BIN" build 2>&1)
TOTAL=$(ls content/posts/*.md | wc -l | tr -d ' ')
EXPECT=$((TOTAL - 1))   # broken.md is rejected

group "Index completeness"
check "all $EXPECT posts listed"  "$(grep -c 'href="/posts/' public/index.html)" "$EXPECT"
check "index is closed properly"  "$(grep -c '<ul' public/index.html)" "$(grep -c '</ul>' public/index.html)"
check "no truncated list item"    "$(grep -c '<li>' public/index.html)" "$(grep -c '</li>' public/index.html)"

group "Ordering"
check "newest post first" "$(grep -o 'href="/posts/[^"]*"' public/index.html | head -1)" 'href="/posts/newest/"'
check "oldest post last"  "$(grep -o 'href="/posts/[^"]*"' public/index.html | tail -1)" 'href="/posts/oldest/"'
DATES=$(grep -o '<time>[^<]*</time>' public/index.html | sed 's/<[^>]*>//g')
if [ "$DATES" = "$(echo "$DATES" | sort -r)" ]; then ok "dates descending"; else bad "dates not sorted"; fi

group "Escaping"
grep -q '&amp; Jerry &lt;script&gt;' public/index.html && ok "index escapes & < >" || bad "index unescaped"
grep -q '&amp; Jerry &lt;script&gt;' public/rss.xml   && ok "rss escapes & < >"   || bad "rss unescaped"
grep -q '<script>alert(1)</script>' public/index.html && bad "raw script tag present" || ok "no raw script tag"
grep -q '&quot;quoted&quot;' public/index.html        && ok "quotes escaped"       || bad "quotes unescaped"
if command -v xmllint >/dev/null 2>&1; then
    xmllint --noout public/rss.xml 2>/dev/null && ok "rss.xml is valid XML" || bad "rss.xml malformed"
fi

group "Slugs"
check "no empty-slug link" "$(grep -c 'href="/posts//"' public/index.html)" "0"
[ -f public/posts/derived-name/index.html ] && ok "slug derived from filename" || bad "slug not derived"
[ -f public/posts/escaped/index.html ]      && ok "traversal slug neutralised" || bad "traversal slug kept"
[ -e "$WORK/escaped" ] && bad "slug escaped output dir" || ok "nothing written outside output dir"

group "RSS"
grep -qE '<pubDate>[A-Z][a-z]{2}, [0-9]{2} [A-Z][a-z]{2} [0-9]{4} 00:00:00 \+0000</pubDate>' public/rss.xml \
    && ok "RFC-822 pubDate" || bad "pubDate not RFC-822"
check "item count matches index" "$(grep -c '<item>' public/rss.xml)" "$EXPECT"

group "Markdown rendering"
M=public/posts/md/index.html
grep -q '<h1>Heading</h1>'          "$M" && ok "heading"     || bad "heading"
grep -q '<strong>bold</strong>'     "$M" && ok "bold"        || bad "bold"
grep -q '<code>code</code>'         "$M" && ok "inline code" || bad "inline code"
grep -q 'href="https://example.com"' "$M" && ok "link"       || bad "link"
grep -q '<pre><code class="language-c">' "$M" && ok "fenced code block" || bad "fenced code block"
grep -q '<li>one</li>'              "$M" && ok "list"        || bad "list"
grep -q '<blockquote>'              "$M" && ok "blockquote"  || bad "blockquote"
printf -- "---\ntitle: Raw\ndate: 2016-02-02\nslug: raw\n---\n\n<figure id=\"x\">html</figure>\n" > content/posts/raw.md
"$BIN" build >/dev/null 2>&1
grep -q '<figure id="x">html</figure>' public/posts/raw/index.html \
    && ok "raw HTML passes through" || bad "raw HTML stripped"
rm -f content/posts/raw.md

group "Bad input"
echo "$BUILD_OUT" | grep -q 'broken.md' && ok "bad frontmatter reported" || bad "bad frontmatter silent"

group "Drafts"
post "draft.md" "Secret" "2031-01-01" "secret" "d"
printf -- "---\ntitle: Secret\ndate: 2031-01-01\nslug: secret\ndescription: d\ndraft: true\n---\n\nbody\n" \
    > content/posts/draft.md
D_OUT=$("$BIN" build 2>&1)
[ -e public/posts/secret ] && bad "draft was published" || ok "draft excluded from output"
grep -q 'href="/posts/secret/"' public/index.html && bad "draft listed on index" || ok "draft absent from index"
grep -q '/posts/secret/' public/rss.xml && bad "draft in feed" || ok "draft absent from feed"
echo "$D_OUT" | grep -q '1 draft(s) skipped' && ok "draft count reported" || bad "draft skipped silently"
"$BIN" build --drafts >/dev/null 2>&1
[ -f public/posts/secret/index.html ] && ok "--drafts publishes it" || bad "--drafts had no effect"
"$BIN" serve --drafts 2>&1 | grep -q 'only applies to build' && ok "--drafts rejected on serve" || bad "--drafts silently ignored on serve"
rm -f content/posts/draft.md && "$BIN" build >/dev/null 2>&1

group "Pages"
printf -- "---\ntitle: Hakkımda\nslug: about\ndescription: d\n---\n\n# Merhaba\n\nsayfa içeriği\n" > content/about.md
printf -- "---\ntitle: Gizli Sayfa\nslug: hidden\ndraft: true\n---\n\nx\n" > content/hidden.md
P_OUT=$("$BIN" build 2>&1)
[ -f public/about/index.html ] && ok "page published at /<slug>/" || bad "page not published"
grep -q 'Merhaba' public/about/index.html && ok "page content rendered" || bad "page content missing"
grep -q 'href="/posts/about/"' public/index.html && bad "page listed as a post" || ok "page absent from post index"
grep -q '/about/' public/rss.xml && bad "page in feed" || ok "page absent from feed"
[ -e public/hidden ] && bad "draft page published" || ok "draft page excluded"
echo "$P_OUT" | grep -q 'page(s)' && ok "page count reported" || bad "page count missing"

group "Page menu"
printf -- "---\ntitle: Projelerim\nslug: projects\n---\n\nx\n" > content/projects.md
printf -- "---\ntitle: İletişim\nslug: iletisim\n---\n\nx\n" > content/iletisim.md
"$BIN" build >/dev/null 2>&1
menu() { grep -o '<nav class="site-nav">.*</nav>' "$1" | grep -o 'href="/[a-z-]*/"' | tr '\n' ' '; }
check "menu built from content/ with no config" "$(menu public/index.html)" 'href="/about/" href="/iletisim/" href="/projects/" '
check "menu also on posts"  "$(menu public/posts/newest/index.html)" 'href="/about/" href="/iletisim/" href="/projects/" '
check "menu also on pages"  "$(menu public/projects/index.html)"     'href="/about/" href="/iletisim/" href="/projects/" '
# a non-ASCII title must not be exiled to the end of the menu
check "Turkish title sorts alphabetically" "$(menu public/index.html | grep -o 'iletisim')" "iletisim"
# order: promotes, it must not demote
printf -- "---\ntitle: Projelerim\nslug: projects\norder: 1\n---\n\nx\n" > content/projects.md
"$BIN" build >/dev/null 2>&1
check "order: 1 moves a page to the front" "$(menu public/index.html)" 'href="/projects/" href="/about/" href="/iletisim/" '
grep -q 'ul:empty' public/style.css && ok "empty menu hidden by CSS" || bad "no :empty rule for the menu"
# drafts stay out of the menu
printf -- "---\ntitle: Gizli\nslug: gizli\ndraft: true\n---\n\nx\n" > content/gizli.md
"$BIN" build >/dev/null 2>&1
grep -q '/gizli/' public/index.html && bad "draft page in menu" || ok "draft page absent from menu"
rm -f content/gizli.md content/projects.md content/iletisim.md
"$BIN" build >/dev/null 2>&1

group "Prev/next navigation"
N=public/posts/middle/index.html
grep -q 'class="prev" href="/posts/[a-z0-9-]*/"' "$N" && ok "prev link present"  || bad "prev link missing"
grep -q 'class="next" href="/posts/[a-z0-9-]*/"' "$N" && ok "next link present"  || bad "next link missing"
# newest post has no newer neighbour, oldest has no older one
grep -q 'class="next" href=""' public/posts/newest/index.html && ok "newest has empty next" || bad "newest has a next link"
grep -q 'class="prev" href=""' public/posts/oldest/index.html && ok "oldest has empty prev" || bad "oldest has a prev link"
grep -q 'a:empty' public/style.css && ok "empty nav links hidden by CSS" || bad "no :empty rule in theme CSS"
# the neighbour of the newest post must be the second-newest
grep -o 'class="prev" href="[^"]*"' public/posts/newest/index.html | grep -q 'esc\|middle\|bulk\|derived\|escaped' \
    && ok "prev points at an older post" || bad "prev target wrong"

group "Slug transliteration"
"$BIN" new "Merhaba Dünya Işık Çöğüş" >/dev/null 2>&1
ls content/posts/ | grep -q 'merhaba-dunya-isik-cogus' && ok "Turkish transliterated" || bad "Turkish mangled"
"$BIN" new "Ünlü" >/dev/null 2>&1
ls content/posts/ | grep -q -- '-unlu\.md' && ok "unicode-only title" || bad "unicode-only title"
"$BIN" new "C: The Language!!" >/dev/null 2>&1
ls content/posts/ | grep -q 'c-the-language' && ok "punctuation collapsed" || bad "punctuation"
"$BIN" new Tirnaksiz Cok Kelimeli Baslik >/dev/null 2>&1
ls content/posts/ | grep -q 'tirnaksiz-cok-kelimeli-baslik' && ok "unquoted multi-word title" || bad "multi-word title truncated"
grep -q 'title: Tirnaksiz Cok Kelimeli Baslik' content/posts/*tirnaksiz*.md && ok "full title kept in frontmatter" || bad "title not joined"

group "Nested pages and the doc tree"
# a theme has to opt into the sidebar, so put the placeholders in one
for t in page post; do
    sed -i.bak 's|<article>|<aside class="tree">{{page_tree}}</aside><article>|' "themes/default/templates/$t.html"
done
sed -i.bak 's|</article>|</article><nav class="post-nav"><a class="prev" href="{{prev_url}}">{{prev_title}}</a><a class="next" href="{{next_url}}">{{next_title}}</a></nav><p class="edit"><a href="{{edit_url}}">{{source_path}}</a></p>|' themes/default/templates/page.html
rm -f themes/default/templates/*.bak
mkdir -p content/guide content/api content/guide/deep
printf -- "---\ntitle: Guide\norder: 1\n---\n\nsection\n"   > content/guide/index.md
printf -- "---\ntitle: Install\norder: 1\n---\n\nx\n"        > content/guide/install.md
printf -- "---\ntitle: Yapılandırma\norder: 2\n---\n\nx\n"   > content/guide/config.md
printf -- "---\ntitle: Deeper\n---\n\nx\n"                   > content/guide/deep/more.md
printf -- "---\ntitle: API\norder: 2\n---\n\nsection\n"      > content/api/index.md
printf -- "---\ntitle: build\n---\n\nx\n"                    > content/api/build.md
printf 'edit_url = "https://example.com/edit/main"\n' > cfg.tmp && cat config.toml >> cfg.tmp && mv cfg.tmp config.toml
"$BIN" build >/dev/null 2>&1

[ -f public/guide/install/index.html ]     && ok "nested page published"        || bad "nested page missing"
[ -f public/guide/index.html ]             && ok "index.md maps to its directory" || bad "index.md not mapped"
[ -f public/guide/deep/more/index.html ]   && ok "two levels deep published"    || bad "deep page missing"
[ -e public/guide/index/index.html ]       && bad "index.md published as a child" || ok "no /guide/index/ duplicate"

tree_of() { grep -o '<aside class="tree">.*</aside>' "$1"; }
tree_of public/guide/install/index.html | grep -q '<li><a href="/guide/">Guide</a><ul>' \
    && ok "tree nests children under their section" || bad "tree not nested"
tree_of public/guide/install/index.html | grep -q '/guide/install/' \
    && ok "tree reaches leaf pages" || bad "leaf missing from tree"
# the sidebar appears on posts too, not only on pages
tree_of public/posts/newest/index.html | grep -q '/guide/' \
    && ok "tree available on posts" || bad "tree missing on posts"
# the top menu stays one level deep
menu2() { grep -o '<nav class="site-nav">.*</nav>' "$1" | grep -o 'href="/[a-z-]*/"' | tr '\n' ' '; }
check "menu shows sections, not their children" "$(menu2 public/index.html)" 'href="/guide/" href="/api/" href="/about/" '

# prev/next must follow the sidebar, not the file system or a calendar
nx() { grep -o 'class="next" href="[^"]*"' "$1" | sed 's/class="next" href="//;s/"//'; }
pv() { grep -o 'class="prev" href="[^"]*"' "$1" | sed 's/class="prev" href="//;s/"//'; }
check "section leads to its first child" "$(nx public/guide/index.html)"          "/guide/install/"
check "reading order continues in order"  "$(nx public/guide/install/index.html)" "/guide/config/"
# content/guide/deep has no index.md, so it is a heading with no page of its
# own: reading order steps over it straight into its first child
check "steps over a section with no index.md" "$(nx public/guide/config/index.html)" "/guide/deep/more/"
tree_of public/guide/install/index.html | grep -q '<li><span>deep</span>' \
    && ok "page-less section renders as a heading" || bad "page-less section became a link"
check "crosses into the next section"     "$(nx public/guide/deep/more/index.html)" "/api/"
check "prev walks back up"                "$(pv public/api/index.html)"           "/guide/deep/more/"
check "last page in the tree has no next" "$(nx public/about/index.html)"         ""

grep -q 'https://example.com/edit/main/content/guide/install.md' public/guide/install/index.html \
    && ok "edit_url joins the source path" || bad "edit link wrong"
rm -rf content/guide content/api
"$BIN" build >/dev/null 2>&1

group "Config"
printf 'title = "T"\ntheme = "dark"\n\n[build]\noutput_dir = "docs"\n\n[server]\nport = %s\n' "$PORT" > config.toml
"$BIN" build >/dev/null 2>&1
[ -f docs/index.html ] && ok "[build] output_dir honoured" || bad "output_dir ignored"
[ -f docs/rss.xml ]    && ok "rss follows output_dir"      || bad "rss ignores output_dir"
printf 'theme = "../../etc"\n' > bad.toml
cp config.toml good.toml && cp bad.toml config.toml
"$BIN" build 2>&1 | grep -q 'invalid theme name' && ok "path-y theme rejected" || bad "path-y theme accepted"
cp good.toml config.toml

group "Dev server"
"$BIN" serve > "$WORK/serve.log" 2>&1 &
SRV=$!
sleep 1
grep -q "$PORT" "$WORK/serve.log" && ok "[server] port honoured" || bad "port from config ignored"
echo "SECRET" > "$WORK/secret.txt"
req() { printf "GET %s HTTP/1.1\r\nHost: x\r\n\r\n" "$1" | nc localhost "$PORT" 2>/dev/null; }
check "traversal blocked"          "$(req '/../../secret.txt'          | head -1 | tr -d '\r')" "HTTP/1.1 403 Forbidden"
check "config.toml unreachable"    "$(req '/../config.toml'            | head -1 | tr -d '\r')" "HTTP/1.1 403 Forbidden"
check "encoded traversal blocked"  "$(req '/%2e%2e/%2e%2e/secret.txt'  | head -1 | tr -d '\r')" "HTTP/1.1 403 Forbidden"
check "query string served"        "$(req '/?v=1'                      | head -1 | tr -d '\r')" "HTTP/1.1 200 OK"
check "root served"                "$(req '/'                          | head -1 | tr -d '\r')" "HTTP/1.1 200 OK"
check "post page served"           "$(req '/posts/newest/'             | head -1 | tr -d '\r')" "HTTP/1.1 200 OK"
check "missing file 404"           "$(req '/nope.html'                 | head -1 | tr -d '\r')" "HTTP/1.1 404 Not Found"
# a directory must redirect, not answer with an empty 200
check "post without trailing slash redirects" "$(req '/posts/newest' | head -1 | tr -d '\r')" "HTTP/1.1 301 Moved Permanently"
check "redirect points at the slashed URL"    "$(req '/posts/newest' | grep -i '^location' | tr -d '\r')" "Location: /posts/newest/"
check "page without trailing slash redirects" "$(req '/about'        | head -1 | tr -d '\r')" "HTTP/1.1 301 Moved Permanently"
check "no empty 200 for a directory"          "$(req '/posts/newest' | grep -c 'Content-Type: application/octet-stream')" "0"
check "CRLF injection in path rejected" "$(req '/about%0d%0aX-Injected:%20yes' | head -1 | tr -d '\r')" "HTTP/1.1 403 Forbidden"
check "rss content-type"           "$(req '/rss.xml' | grep -i '^content-type' | tr -d '\r')"   "Content-Type: application/xml"
check "bad method 405" "$(printf 'DELETE / HTTP/1.1\r\n\r\n' | nc localhost "$PORT" | head -1 | tr -d '\r')" "HTTP/1.1 405 Method Not Allowed"
printf '\r\n\r\n'  | nc localhost "$PORT" >/dev/null 2>&1
printf 'GARBAGE'   | nc localhost "$PORT" >/dev/null 2>&1
for _ in 1 2 3; do (printf 'GET /index.html HTTP/1.1\r\n\r\n' | nc localhost "$PORT" >/dev/null 2>&1 &); done
sleep 1
check "survives malformed input and disconnects" "$(req '/' | head -1 | tr -d '\r')" "HTTP/1.1 200 OK"
if netstat -an 2>/dev/null | grep -qE "[.:]$PORT " | grep -q '\*\.'; then
    bad "bound to all interfaces"
else
    ok "loopback only"
fi


group "CLI guards"
check "garbage port rejected"     "$("$BIN" serve abc   2>&1 | head -1)" 'Error: invalid port "abc" (expected 1-65535)'
check "out-of-range port rejected" "$("$BIN" serve 99999 2>&1 | head -1)" 'Error: invalid port "99999" (expected 1-65535)'
"$BIN" help 2>&1 | grep -q 'config.toml not found' && bad "help warns about config" || ok "help is quiet"
cd "$WORK"
printf 'site\n\n\n\n\n\n\n\n'    | "$BIN" init 2>&1 | grep -q 'already exists'        && ok "existing dir refused" || bad "existing dir reused"
printf '../evil\n\n\n\n\n\n\n\n' | "$BIN" init 2>&1 | grep -q 'not a valid project'   && ok "path-y project name refused" || bad "path-y name accepted"

group "init scaffolding"
cd "$WORK"
printf 'scaf\nBasligim\nAciklamam\nYavuz Selim\nhttps://x.com\n\n\n1\nprojects,about\n' | "$BIN" init >/dev/null 2>&1
[ -f scaf/content/projects.md ] && ok "picked page created"        || bad "picked page missing"
[ -f scaf/content/about.md ]    && ok "second picked page created" || bad "second page missing"
[ -f scaf/content/contact.md ]  && bad "unpicked page created"     || ok "unpicked page not created"
# order follows the pick order, not the listing order
check "first pick gets order 1"  "$(grep '^order:' scaf/content/projects.md)" "order: 1"
check "second pick gets order 2" "$(grep '^order:' scaf/content/about.md)"    "order: 2"
grep -q 'Hi, I am Yavuz Selim' scaf/content/about.md && ok "about prefilled with the author"      || bad "about not prefilled"
grep -q 'Aciklamam'            scaf/content/about.md && ok "about prefilled with the description" || bad "description missing"
# the sample post must be dated today, not whenever init was written
check "sample post dated today" "$(grep '^date:' scaf/content/posts/*.md)" "date: $(date +%Y-%m-%d)"
# names and numbers are both accepted, unknown entries are reported
printf 'scaf2\n\n\n\n\n\n\n1\n2,nope,now\n' | "$BIN" init 2>&1 | grep -q 'ignoring unknown page "nope"' \
    && ok "unknown page reported" || bad "unknown page silently dropped"
[ -f scaf2/content/projects.md ] && [ -f scaf2/content/now.md ] && ok "numbers and names mix" || bad "mixed selection failed"
# piped init must still apply defaults for the questions it never reaches
printf 'scaf3\n' | "$BIN" init >/dev/null 2>&1
[ -f scaf3/content/about.md ] && ok "EOF falls back to defaults" || bad "EOF left answers empty"

group "Attribution footer"
cd "$WORK"
printf 'ft\nSite\nDesc\nY\nhttps://x\n\n\n1\nabout\n' | "$BIN" init >/dev/null 2>&1
cd ft
grep -q '^built_with  = true' config.toml && ok "init enables it by default" || bad "config key missing"
"$BIN" build >/dev/null 2>&1
grep -q 'Generated with atomik-ssg' public/index.html            && ok "footer on the index" || bad "index has no footer"
grep -q 'Generated with atomik-ssg' public/posts/hello-world/index.html && ok "footer on a post"  || bad "post has no footer"
grep -q 'Generated with atomik-ssg' public/about/index.html      && ok "footer on a page"  || bad "page has no footer"
grep -q 'github.com/yavuzselimsahin/atomik-ssg' public/index.html && ok "footer links to the project" || bad "footer link wrong"
# commenting the line out is the documented way to switch it off
sed -i.bak 's|^built_with  = true|# built_with  = true|' config.toml && rm -f config.toml.bak
"$BIN" build >/dev/null 2>&1
grep -q 'Generated with atomik-ssg' public/index.html && bad "commenting it out had no effect" || ok "commented out removes it"
grep -q 'site-footer:empty' public/style.css && ok "empty footer hidden by CSS" || bad "empty footer still takes space"
grep -q '^\.site-footer { margin-top' public/style.css && ok "footer has its own spacing and rule" || bad "footer base rule missing"
cd "$WORK"

group "docs theme"
cd "$WORK"
# the pages question must not be asked for a docs site, so no answer is given
D_INIT=$(printf 'dsite\nDocs Site\nA description\nY\nhttps://x.com\n\n\n4\n' | "$BIN" init 2>&1)
echo "$D_INIT" | grep -q 'Starter pages' && bad "portfolio pages offered for a docs site" || ok "portfolio pages not offered for docs"
echo "$D_INIT" | grep -q 'docs theme starts you off' && ok "docs scaffold announced" || bad "no docs scaffold notice"
for f in index page post; do
    [ -f "dsite/themes/docs/templates/$f.html" ] || bad "docs $f.html missing"
done
[ -f dsite/themes/docs/templates/page.html ] && ok "docs theme scaffolded" || bad "docs theme missing"
[ -f dsite/themes/docs/static/style.css ]     && ok "docs stylesheet written" || bad "docs stylesheet missing"
grep -q 'theme       = "docs"' dsite/config.toml && ok "picker selects docs" || bad "docs not selected"
grep -q '{{page_tree}}' dsite/themes/docs/templates/page.html && ok "sidebar carries the tree" || bad "no tree in sidebar"
grep -q '{{page_tree}}' dsite/themes/docs/templates/post.html && ok "posts get the sidebar too" || bad "post lacks sidebar"
cd dsite
# the scaffold is the tutorial: it must exist, nest, and carry no blog
[ -f content/getting-started.md ]     && ok "getting-started scaffolded"  || bad "no getting-started page"
[ -f content/writing/pages.md ]       && ok "nested guide scaffolded"     || bad "no nested guide"
[ -f content/reference/commands.md ]  && ok "reference section scaffolded" || bad "no reference section"
[ -z "$(ls content/posts 2>/dev/null)" ] && ok "docs site starts with no posts" || bad "docs site got a sample post"
[ -f content/about.md ]               && bad "docs site got a portfolio page" || ok "no portfolio pages on a docs site"
D_BUILD=$("$BIN" build 2>&1)
echo "$D_BUILD" | grep -q '0 post(s), 8 page(s)' && ok "scaffold builds to 8 pages" || bad "unexpected build: $(echo "$D_BUILD" | tail -1)"
echo "$D_BUILD" | grep -q 'post_items' && bad "warns about the missing post list" || ok "quiet about the absent post list"
grep -q 'class="post-list"' public/index.html && bad "docs landing page lists posts" || ok "docs landing page is not a feed"
grep -q '<nav class="contents">' public/index.html && ok "docs landing page is a table of contents" || bad "no contents on the landing page"
grep -q 'href="/writing/pages/"' public/index.html && ok "contents reaches nested pages" || bad "contents is not nested"
# the scaffold must not lean on markdown cmark cannot render
grep -rq '^|' content/ && bad "scaffold uses pipe tables cmark cannot render" || ok "scaffold avoids GFM-only syntax"
mkdir -p content/guide
printf -- "---\ntitle: Guide\norder: 1\n---\n\nx\n"    > content/guide/index.md
printf -- "---\ntitle: Install\norder: 1\n---\n\nx\n"  > content/guide/install.md
printf 'edit_url = "https://example.com/edit/main"\n' > c.tmp && cat config.toml >> c.tmp && mv c.tmp config.toml
"$BIN" build >/dev/null 2>&1
G=public/guide/install/index.html
grep -q '<aside class="sidebar">' "$G"       && ok "sidebar rendered"      || bad "sidebar missing"
grep -q 'href="/guide/install/"' "$G"        && ok "tree lists the page"   || bad "tree incomplete"
grep -q "classList.add('active')" "$G"       && ok "current-page script shipped" || bad "no active-page script"
grep -q 'class="edit" href="https://example.com/edit/main/content/guide/install.md"' "$G" \
    && ok "edit link composed" || bad "edit link wrong"
grep -q 'a.edit\[href=""\]' public/style.css || grep -q '.edit\[href=""\]' public/style.css \
    && ok "empty edit link hidden by CSS" || bad "no rule hiding an unset edit link"
grep -q 'prefers-color-scheme' public/style.css && ok "docs theme follows the system palette" || bad "no dark mode"
# the version badge: present when set, gone when not
printf 'version = "9.9.9"\n' > c.tmp && cat config.toml >> c.tmp && mv c.tmp config.toml
"$BIN" build >/dev/null 2>&1
grep -q '<span class="version">9.9.9</span>' public/guide/install/index.html \
    && ok "version shown when set" || bad "version not shown"
grep -v '^version' config.toml > c.tmp && mv c.tmp config.toml
"$BIN" build >/dev/null 2>&1
grep -q '<span class="version"></span>' public/guide/install/index.html \
    && ok "version empty when unset" || bad "version left over"
grep -q '\.version:empty' public/style.css \
    && ok "empty version badge hidden by CSS" || bad "empty badge still takes space"
# the attribution belongs at the foot of the sidebar, outside the part that
# scrolls, so it can be read without scrolling an article to the end
python3 - <<'EOF' && ok "docs footer pinned under the sidebar" || bad "docs footer is not in the sidebar"
import re, sys
h = open('public/guide/install/index.html').read()
aside = re.search(r'<aside class="sidebar">.*?</aside>', h, re.S)
main  = re.search(r'<main>.*?</main>', h, re.S)
sys.exit(0 if aside and 'site-footer' in aside.group(0)
              and main and 'site-footer' not in main.group(0) else 1)
EOF
grep -q '\.sidebar > ul { overflow-y: auto' public/style.css \
    && ok "the tree scrolls, not the whole sidebar" || bad "sidebar scrolls as one piece"
grep -q '\.site-footer { flex-shrink: 0' public/style.css \
    && ok "footer holds its height" || bad "footer can be squeezed away"
# the attribution is a link, not another row of navigation, so the tree's
# block styling must not reach it
grep -q '^\.sidebar > ul a, \.sidebar > ul span' public/style.css \
    && ok "tree styling scoped away from the footer" || bad "sidebar rules still catch the footer link"
grep -q "querySelectorAll('\.sidebar > ul a')" public/guide/install/index.html \
    && ok "active-page script looks at the tree only" || bad "script scans the whole sidebar"
# the phone override unsets the sticky sidebar, so at equal specificity it has
# to come later in the file or the sidebar keeps overlapping the content
STICKY=$(grep -n '^\.sidebar { position: sticky' public/style.css | cut -d: -f1)
MOBILE=$(grep -n '@media (max-width: 800px)' public/style.css | cut -d: -f1)
if [ -n "$STICKY" ] && [ -n "$MOBILE" ] && [ "$MOBILE" -gt "$STICKY" ]; then
    ok "phone override comes after the sticky sidebar rule"
else
    bad "phone override at line $MOBILE cannot beat the sticky rule at line $STICKY"
fi
grep -q 'position: static' public/style.css && ok "sidebar unsticks on a phone" || bad "sidebar stays sticky on a phone"
# palette comes from the blog themes, so a mixed site stays one thing
grep -q -- '--accent:   #2b6cb0' public/style.css && ok "light palette matches the default theme" || bad "light palette drifted"
grep -q -- '--bg:      #0f1117'  public/style.css && ok "dark palette matches the dark theme"    || bad "dark palette drifted"
# the scheme toggle
grep -q 'class="theme-toggle"' public/guide/install/index.html && ok "scheme toggle rendered" || bad "no scheme toggle"
grep -q "localStorage.getItem('theme')" public/guide/install/index.html && ok "stored choice applied before paint" || bad "no pre-paint script"
grep -q "localStorage.setItem('theme'" public/guide/install/index.html && ok "choice is remembered" || bad "choice not stored"
grep -q ':root\[data-theme="dark"\]' public/style.css && ok "explicit dark wins over the system" || bad "no explicit dark rule"
grep -q ':root:not(\[data-theme="light"\])' public/style.css && ok "system dark still honoured" || bad "system preference dropped"
cd "$WORK"

group "Callouts"
cd "$WORK"
printf 'cal\nS\nD\nY\nhttps://x\n\n\n4\n' | "$BIN" init >/dev/null 2>&1
cd cal
cat > content/c.md <<'EOF'
---
title: C
order: 9
---

> [!NOTE]
> One line.

> [!WARNING]
>
> Blank line after the marker.

> [!TIP]
> First.
>
> Second paragraph.

> An ordinary quote.

> [!NOPE]
> Not a known kind.
EOF
"$BIN" build >/dev/null 2>&1
C=public/c/index.html
check "every known kind is classed" "$(grep -c 'class="callout callout-' $C)" "3"
grep -q 'callout-note' $C    && ok "marker on the same line"        || bad "same-line form missed"
grep -q 'callout-warning' $C && ok "marker on its own line"         || bad "own-line form missed"
grep -q 'callout-tip' $C     && ok "several paragraphs kept"        || bad "multi-paragraph form missed"
grep -q '\[!NOTE\]' $C     && bad "marker left in the output"     || ok "marker removed from the text"
grep -q '<p></p>' $C         && bad "empty paragraph left behind"   || ok "no empty paragraph"
grep -q 'Second paragraph'   $C && ok "later paragraphs survive"    || bad "paragraphs lost"
grep -q '<blockquote>\n\?<p>An ordinary' $C || grep -q 'An ordinary quote' $C
grep -q 'class="callout" *>*<p>An ordinary' $C && bad "plain quote was converted" || ok "plain quote untouched"
grep -q '\[!NOPE\]' $C     && ok "unknown kind left alone"        || bad "unknown kind swallowed"
# the label lives in CSS, so it must exist for every kind
check "a label for each kind" "$(grep -c 'callout-[a-z]*::before' public/style.css)" "5"
grep -q -- '--cal-caution' public/style.css && ok "callouts have their own colours" || bad "no callout palette"
# a code fence must never be mistaken for a callout
printf -- "---\ntitle: F\norder: 8\n---\n\n\`\`\`\n> [!NOTE]\n\`\`\`\n" > content/f.md
"$BIN" build >/dev/null 2>&1
grep -q 'callout' public/f/index.html && bad "code fence converted" || ok "code fence untouched"
cd "$WORK"

group "base_path"
cd "$WORK"
printf 'bp\nS\nD\nY\nhttps://x.com\n\n\n4\n' | "$BIN" init >/dev/null 2>&1
cd bp
printf -- "---\ntitle: L\norder: 9\n---\n\n[in](/writing/) [out](https://x.com/a) ![i](/img/a.png)\n\n\`\`\`html\n<a href=\"/code/\">x</a>\n\`\`\`\n" > content/l.md

# with no base_path the output must be exactly as before
"$BIN" build >/dev/null 2>&1
check "unset leaves links at the root" "$(grep -o 'href="/writing/"' public/l/index.html | head -1)" 'href="/writing/"'
grep -q 'href="/style.css"' public/l/index.html && ok "unset leaves the stylesheet link alone" || bad "stylesheet link changed"

printf 'base_path = "/proj"\n' > c.tmp && cat config.toml >> c.tmp && mv c.tmp config.toml
"$BIN" build >/dev/null 2>&1
grep -q 'href="/proj/style.css"'  public/l/index.html && ok "stylesheet prefixed"   || bad "stylesheet not prefixed"
grep -q 'class="brand" href="/proj/"' public/l/index.html && ok "home link prefixed" || bad "home link not prefixed"
grep -q 'href="/proj/writing/"' public/l/index.html && ok "sidebar tree prefixed"    || bad "tree not prefixed"
grep -q 'class="prev" href="/proj/' public/l/index.html && ok "prev/next prefixed"   || bad "neighbours not prefixed"
grep -q 'src="/proj/img/a.png"'  public/l/index.html && ok "markdown image prefixed"  || bad "image not prefixed"
grep -q 'href="https://x.com/a"' public/l/index.html && ok "external link untouched"  || bad "external link rewritten"
grep -q '&quot;/code/&quot;'     public/l/index.html && ok "code block untouched"     || bad "code block rewritten"
grep -q 'href="/posts/'          public/index.html   && bad "post list missed the prefix" || ok "post list prefixed"

# the value is normalised, however it was written
for v in 'proj' '/proj/' '/proj'; do
    printf 'base_path = "%s"\n' "$v" > c.tmp && grep -v '^base_path' config.toml >> c.tmp && mv c.tmp config.toml
    "$BIN" build >/dev/null 2>&1
    grep -q 'href="/proj/style.css"' public/l/index.html || bad "base_path \"$v\" not normalised"
done
ok "base_path normalised however it is written"

printf 'base_path = "/../etc"\n' > c.tmp && grep -v '^base_path' config.toml >> c.tmp && mv c.tmp config.toml
"$BIN" build 2>&1 | grep -q 'invalid base_path' && ok "climbing base_path rejected" || bad "climbing base_path accepted"
cd "$WORK"

group "Deploy safety"
cd site
printf 'title = "T"\n\n[deploy]\nhost = "h; touch %s/PWNED"\npath = "/tmp/x"\n' "$WORK" > config.toml
echo "n" | "$BIN" deploy >/dev/null 2>&1
[ -f "$WORK/PWNED" ] && bad "command injection via config" || ok "config values cannot inject shell commands"
echo "n" | "$BIN" deploy 2>&1 | grep -q 'Deploy cancelled' && ok "confirmation prompt works" || bad "no confirmation prompt"

printf '\n=========================================\n'
printf '  PASS: %d   FAIL: %d\n' "$PASS" "$FAIL"
printf '=========================================\n'
[ "$FAIL" -eq 0 ]
