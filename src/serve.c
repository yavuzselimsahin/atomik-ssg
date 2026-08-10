#include "../include/serve.h"
#include "../include/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define closesocket close
#endif

const char *get_mime(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0) return "text/html; charset=utf-8";
    if (strcmp(ext, ".css")  == 0) return "text/css";
    if (strcmp(ext, ".js")   == 0) return "application/javascript";
    if (strcmp(ext, ".png")  == 0) return "image/png";
    if (strcmp(ext, ".jpg")  == 0) return "image/jpeg";
    if (strcmp(ext, ".ico")  == 0) return "image/x-icon";
    return "text/plain";
}

void send_response(int client, int status, const char *mime,
                   const char *body, size_t body_len) {
    char header[512];
    const char *status_text = (status == 200) ? "OK" : "Not Found";
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, status_text, mime, body_len);
    send(client, header, strlen(header), 0);
    if (body && body_len > 0)
        send(client, body, (int)body_len, 0);
}

void cmd_serve(int port) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    int server_fd;
    struct sockaddr_in addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Error: port %d unavailable\n", port);
        return;
    }

    listen(server_fd, 10);
    printf("Server running: http://localhost:%d\n", port);
    printf("Press Ctrl+C to stop\n\n");

    char buf[2048];
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client < 0) continue;

        int received = recv(client, buf, sizeof(buf) - 1, 0);
        if (received <= 0) { closesocket(client); continue; }
        buf[received] = '\0';

        char method[8], url_path[512];
        sscanf(buf, "%7s %511s", method, url_path);
        printf("%s %s\n", method, url_path);

        char file_path[600];
        if (strcmp(url_path, "/") == 0)
            snprintf(file_path, sizeof(file_path), "public/index.html");
        else if (url_path[strlen(url_path)-1] == '/')
            snprintf(file_path, sizeof(file_path), "public%sindex.html", url_path);
        else
            snprintf(file_path, sizeof(file_path), "public%s", url_path);

        FILE *fp = fopen(file_path, "rb");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            rewind(fp);
            char *body = malloc(size);
            fread(body, 1, size, fp);
            fclose(fp);
            send_response(client, 200, get_mime(file_path), body, size);
            free(body);
        } else {
            const char *not_found = "<h1>404 - Page not found</h1>";
            send_response(client, 404, "text/html", not_found, strlen(not_found));
        }

        closesocket(client);
    }

#ifdef _WIN32
    WSACleanup();
#endif
}