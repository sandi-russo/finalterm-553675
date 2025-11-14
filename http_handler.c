#include "http_handler.h"

// helper per inviare risposte HTTP
static void send_response(int client_fd, int status_code, const char* status_text,
                         const char* content_type, const char* body) {
    char header[512];
    int body_len = body ? strlen(body) : 0;

    // Costruisci l'header HTTP
    int header_len = snprintf(header, sizeof(header),
                             "HTTP/1.1 %d %s\r\n"
                             "Content-Type: %s\r\n"
                             "Content-Length: %d\r\n"
                             "Connection: close\r\n"
                             "\r\n",
                             status_code, status_text, content_type, body_len);

    if (send(client_fd, header, header_len, 0) < 0) {
        perror("send header");
        return;
    }

    if (body && body_len > 0) {
        if (send(client_fd, body, body_len, 0) < 0) {
            perror("send body");
        }
    }
}

// estrae il body dalla richiesta HTTP
static char* extract_body(const char* request) {
    char* body_start = strstr(request, "\r\n\r\n");
    if (body_start) {
        return body_start + 4;
    }
    return NULL;
}

// gestisco la richiesta HTTP
void handle_http_request(void* arg) {
    RequestContext* ctx = (RequestContext*)arg;
    char buffer[BUFFER_SIZE] = {0};

    ssize_t bytes_read = read(ctx->client_fd, buffer, BUFFER_SIZE - 1);

    if (bytes_read <= 0) {
        printf("Client (fd: %d) disconnesso.\n", ctx->client_fd);
        goto cleanup;
    }

    buffer[bytes_read] = '\0';
    printf("Worker %ld gestisce fd: %d\n", pthread_self(), ctx->client_fd);

    // parsing della richiesta
    char method[16], path[256];
    if (sscanf(buffer, "%15s %255s", method, path) != 2) {
        send_response(ctx->client_fd, 400, "Bad Request",
                     "text/plain", "Richiesta malformata");
        goto cleanup;
    }

    printf("Richiesta: %s %s\n", method, path);

    // routing delle richieste
    char* response_body = NULL;
    int status_code = 200;
    const char* status_text = "OK";
    const char* content_type = "application/json";

    // tutti gli utenti
    if (strcmp(method, "GET") == 0 && strcmp(path, "/users") == 0) {
        response_body = db_get_all_users(ctx->db);
        if (!response_body) {
            status_code = 500;
            status_text = "Internal Server Error";
            response_body = strdup("{\"error\":\"Database error\"}");
        }
    }
    // utente specifico
    else if (strcmp(method, "GET") == 0 && strncmp(path, "/users/", 7) == 0) {
        int user_id = atoi(path + 7);
        response_body = db_get_user(ctx->db, user_id);
        if (!response_body) {
            status_code = 404;
            status_text = "Not Found";
            response_body = strdup("{\"error\":\"User not found\"}");
        }
    }
    // crea utente
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/users") == 0) {
        char* body = extract_body(buffer);
        if (body && db_create_user(ctx->db, body)) {
            status_code = 201;
            status_text = "Created";
            response_body = strdup("{\"status\":\"created\"}");
        } else {
            status_code = 400;
            status_text = "Bad Request";
            response_body = strdup("{\"error\":\"Cannot create user\"}");
        }
    }
    // elimina utente
    else if (strcmp(method, "DELETE") == 0 && strncmp(path, "/users/", 7) == 0) {
        int user_id = atoi(path + 7);
        if (db_delete_user(ctx->db, user_id)) {
            response_body = strdup("{\"status\":\"deleted\"}");
        } else {
            status_code = 404;
            status_text = "Not Found";
            response_body = strdup("{\"error\":\"User not found\"}");
        }
    }
    // endpoint non trovato
    else {
        status_code = 404;
        status_text = "Not Found";
        content_type = "text/plain";
        response_body = strdup("Endpoint non trovato");
    }

    // invia la risposta
    send_response(ctx->client_fd, status_code, status_text, content_type,
                 response_body ? response_body : "");

    if (response_body) {
        free(response_body);
    }

cleanup:
    epoll_ctl(ctx->epoll_fd, EPOLL_CTL_DEL, ctx->client_fd, NULL);
    close(ctx->client_fd);
    free(ctx);
}