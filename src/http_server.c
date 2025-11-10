#include "http_server.h"
#include "api_handler.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 2048

void handle_client(int client_sock, Storage* store) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        perror("read");
        close(client_sock);
        return;
    }

    buffer[bytes_read] = '\0';

    // 解析请求行
    char method[8], path[128];
    if (sscanf(buffer, "%7s %127s", method, path) != 2) {
        const char* msg = "{\"error\": \"Malformed request line\"}";
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response),
            "HTTP/1.0 400 Bad Request\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %zu\r\n"
            "\r\n"
            "%s", strlen(msg), msg);
        write(client_sock, response, strlen(response));
        close(client_sock);
        return;
    }

    // 查找请求体
    char* body = strstr(buffer, "\r\n\r\n");
    if (body) {
        body += 4;
    } else {
        body = "";
    }

    // 响应缓存
    char msg[BUFFER_SIZE * 2];
    memset(msg, 0, sizeof(msg));

    // 是否为完整响应（含 HTTP 头），用于区分 HTML vs JSON
    int is_full_http_response = 0;

    // 分发请求
    handle_api_request(store, path, body, msg, sizeof(msg));

    // 如果返回内容以 "HTTP/" 开头，视为完整响应（如 handle_index 返回）
    if (strncmp(msg, "HTTP/", 5) == 0) {
        is_full_http_response = 1;
    }

    if (is_full_http_response) {
        // 已包含完整 HTTP 头
        write(client_sock, msg, strlen(msg));
    } else {
        // 标准 JSON 响应包裹 HTTP 头
        char response[BUFFER_SIZE * 2];
        snprintf(response, sizeof(response),
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %zu\r\n"
            "\r\n"
            "%s", strlen(msg), msg);
        write(client_sock, response, strlen(response));
    }

    close(client_sock);
}

void http_server_start(Storage* store, int port) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) < 0) {
        perror("listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("HTTP server started on port %d...\n", port);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        handle_client(client_sock, store);
    }

    close(server_sock);
}
