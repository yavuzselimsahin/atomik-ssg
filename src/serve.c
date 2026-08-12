#include "../include/serve.h"
#include "../include/parser.h"
#include "../include/globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <signal.h>
    #define closesocket close
    #define INVALID_SOCKET (-1)
#endif

const char *get_mime(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";

    if (strcmp(ext, ".html") == 0) return "text/html; charset=utf-8";
    if (strcmp(ext, ".css")  == 0) return "text/css; charset=utf-8";
    if (strcmp(ext, ".js")   == 0) return "application/javascript; charset=utf-8";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".xml")  == 0) return "application/xml";
    if (strcmp(ext, ".txt")  == 0) return "text/plain; charset=utf-8";
    if (strcmp(ext, ".svg")  == 0) return "image/svg+xml";
    if (strcmp(ext, ".png")  == 0) return "image/png";
    if (strcmp(ext, ".jpg")  == 0) return "image/jpeg";
    if (strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif")  == 0) return "image/gif";
    if (strcmp(ext, ".webp") == 0) return "image/webp";
    if (strcmp(ext, ".ico")  == 0) return "image/x-icon";
    if (strcmp(ext, ".woff") == 0) return "font/woff";
    if (strcmp(ext, ".woff2")== 0) return "font/woff2";
    if (strcmp(ext, ".ttf")  == 0) return "font/ttf";
    if (strcmp(ext, ".pdf")  == 0) return "application/pdf";
    return "application/octet-stream";
}

static int send_all(int sock, const char *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
#ifdef MSG_NOSIGNAL
        int n = (int)send(sock, data + sent, (int)(len - sent), MSG_NOSIGNAL);
#else
        int n = (int)send(sock, data + sent, (int)(len - sent), 0);
#endif
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

void send_response(int client, int status, const char *mime,
                   const char *body, size_t body_len) {
    const char *status_text;
    switch (status) {
        case 200: status_text = "OK";           break;
        case 400: status_text = "Bad Request";  break;
        case 403: status_text = "Forbidden";    break;
        case 405: status_text = "Method Not Allowed"; break;
        default:  status_text = "Not Found";    break;
    }

    char header[512];
    int n = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, status_text, mime, body_len);
    if (n < 0 || n >= (int)sizeof(header)) return;

    if (send_all(client, header, (size_t)n) != 0) return;
    if (body && body_len > 0)
        send_all(client, body, body_len);
}

/* Percent-decodes in place into out. Rejects encoded NUL bytes. */
static int url_decode(const char *in, char *out, size_t out_size) {
    size_t j = 0;
    for (size_t i = 0; in[i]; i++) {
        if (j + 1 >= out_size) return -1;

        if (in[i] == '%') {
            if (!isxdigit((unsigned char)in[i + 1]) || !isxdigit((unsigned char)in[i + 2]))
                return -1;
            char hex[3] = { in[i + 1], in[i + 2], '\0' };
            int  v      = (int)strtol(hex, NULL, 16);
            if (v == 0) return -1;          /* %00 would truncate the path */
            out[j++] = (char)v;
            i += 2;
        } else {
            out[j++] = in[i];
        }
    }
    out[j] = '\0';
    return 0;
}

/* Rejects any path that could escape the output directory. Runs *after*
   decoding, so %2e%2e%2f cannot slip through. */
static int path_is_safe(const char *p) {
    if (p[0] != '/') return 0;
    if (strchr(p, '\\')) return 0;

    const char *s = p;
    while (*s) {
        if (*s == '/') s++;
        const char *seg = s;
        while (*s && *s != '/') s++;
        size_t len = (size_t)(s - seg);
        if (len == 2 && seg[0] == '.' && seg[1] == '.') return 0;
    }
    return 1;
}

/* Turns a raw request target into a path under root. Returns -1 if unsafe. */
static int resolve_path(const char *url, const char *root,
                        char *out, size_t out_size) {
    char trimmed[1024];
    snprintf(trimmed, sizeof(trimmed), "%s", url);

    char *cut = strpbrk(trimmed, "?#");
    if (cut) *cut = '\0';

    char decoded[1024];
    if (url_decode(trimmed, decoded, sizeof(decoded)) != 0) return -1;
    if (!path_is_safe(decoded)) return -1;

    size_t dlen = strlen(decoded);
    int    n;
    if (dlen == 0 || decoded[dlen - 1] == '/')
        n = snprintf(out, out_size, "%s%sindex.html", root, decoded);
    else
        n = snprintf(out, out_size, "%s%s", root, decoded);

    return (n < 0 || n >= (int)out_size) ? -1 : 0;
}

void cmd_serve(int port) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "Error: winsock init failed\n");
        return;
    }
#else
    /* A browser closing a connection mid-response must not kill the server. */
    signal(SIGPIPE, SIG_IGN);
#endif

    int server_fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        perror("socket");
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    /* Loopback only: the dev server exposes the working directory, so it has
       no business being reachable from the local network. */
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port        = htons((unsigned short)port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Error: port %d unavailable\n", port);
        closesocket(server_fd);
        return;
    }
    if (listen(server_fd, 16) < 0) {
        perror("listen");
        closesocket(server_fd);
        return;
    }

    /* Keep the request log readable when stdout is a pipe or a file. */
    setvbuf(stdout, NULL, _IOLBF, 0);

    printf("Server running: http://localhost:%d\n", port);
    printf("Serving: %s/\n", g_output_dir);
    printf("Press Ctrl+C to stop\n\n");

    char buf[8192];
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t          client_len = sizeof(client_addr);

        int client = (int)accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client == INVALID_SOCKET) continue;

        int received = (int)recv(client, buf, sizeof(buf) - 1, 0);
        if (received <= 0) { closesocket(client); continue; }
        buf[received] = '\0';

        char method[16] = {0}, url_path[1024] = {0};
        if (sscanf(buf, "%15s %1023s", method, url_path) != 2) {
            const char *msg = "<h1>400 - Bad request</h1>";
            send_response(client, 400, "text/html; charset=utf-8", msg, strlen(msg));
            closesocket(client);
            continue;
        }

        int is_head = strcmp(method, "HEAD") == 0;
        if (strcmp(method, "GET") != 0 && !is_head) {
            const char *msg = "<h1>405 - Method not allowed</h1>";
            printf("%s %s -> 405\n", method, url_path);
            send_response(client, 405, "text/html; charset=utf-8", msg, strlen(msg));
            closesocket(client);
            continue;
        }

        char file_path[1200];
        if (resolve_path(url_path, g_output_dir, file_path, sizeof(file_path)) != 0) {
            const char *msg = "<h1>403 - Forbidden</h1>";
            printf("%s %s -> 403\n", method, url_path);
            send_response(client, 403, "text/html; charset=utf-8", msg, strlen(msg));
            closesocket(client);
            continue;
        }

        FILE *fp = fopen(file_path, "rb");
        if (!fp) {
            const char *msg = "<h1>404 - Page not found</h1>";
            printf("%s %s -> 404\n", method, url_path);
            send_response(client, 404, "text/html; charset=utf-8", msg, strlen(msg));
            closesocket(client);
            continue;
        }

        long size = -1;
        if (fseek(fp, 0, SEEK_END) == 0) size = ftell(fp);
        if (size < 0) {
            fclose(fp);
            const char *msg = "<h1>404 - Page not found</h1>";
            send_response(client, 404, "text/html; charset=utf-8", msg, strlen(msg));
            closesocket(client);
            continue;
        }
        rewind(fp);

        char *body = malloc((size_t)size + 1);
        if (!body) {
            fclose(fp);
            closesocket(client);
            continue;
        }
        size_t got = fread(body, 1, (size_t)size, fp);
        fclose(fp);

        printf("%s %s -> 200\n", method, url_path);
        send_response(client, 200, get_mime(file_path), is_head ? NULL : body, got);

        free(body);
        closesocket(client);
    }

#ifdef _WIN32
    WSACleanup();
#endif
}
