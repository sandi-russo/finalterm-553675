#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "common.h"
#include "db_handler.h"

typedef struct {
    int client_fd;
    int epoll_fd;
    sqlite3* db;
} RequestContext;

// funzione che esegue il worker per il db
void handle_http_request(void* arg);

#endif