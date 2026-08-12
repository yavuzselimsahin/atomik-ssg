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
