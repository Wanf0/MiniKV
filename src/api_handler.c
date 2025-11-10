#include "api_handler.h"
#include "parser.h"
#include "engine.h"
#include <stdio.h>
#include <string.h>

// 支持 POST /api/query
void handle_api_request(Storage* store, const char* path, const char* body, char* response, int max_len) {
    if (strcmp(path, "/api/query") != 0) {
        snprintf(response, max_len, "{\"error\": \"Unknown API endpoint\"}");
        return;
    }

    // 解析 body 为 KvCommand
    KvCommand cmd;
    if (parse_input(body, &cmd) != 0) {
        snprintf(response, max_len, "{\"error\": \"Invalid query syntax\"}");
        return;
    }

    // 执行命令
    ExecutionResult result = engine_execute(store, &cmd);

    // 构造 JSON 响应（假设 message 为 JSON 片段）
    snprintf(response, max_len, "%s\n", result.message);
}
