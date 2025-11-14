#include "common.h"
#include "thread_pool.h"
#include "http_handler.h"
#include "db_handler.h"

// rendo un socket non bloccante
static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        exit(EXIT_FAILURE);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
    }
}

int main() {
    int server_fd, epoll_fd;
    struct sockaddr_in address;

    // inizializza database
    sqlite3* db = db_init();
    if (!db) {
        fprintf(stderr, "Errore inizializzazione database.\n");
        exit(EXIT_FAILURE);
    }

    // creo il thread pool
    ThreadPool* pool = threadpool_create(NUM_WORKERS);
    if (!pool) {
        fprintf(stderr, "Errore creazione thread pool.\n");
        db_close(db);
        exit(EXIT_FAILURE);
    }

    // creo la socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        goto cleanup_db;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        goto cleanup_socket;
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen");
        goto cleanup_socket;
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        goto cleanup_socket;
    }

    set_nonblocking(server_fd);

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        perror("epoll_ctl: server_fd");
        goto cleanup_epoll;
    }

    printf("Server REST in ascolto sulla porta %d...\n", PORT);

    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if (n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            continue;
        }

        for (int i = 0; i < n; i++) {

            // nuova connessione
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_fd < 0) {
                    perror("accept");
                    continue;
                }

                set_nonblocking(client_fd);

                // EPOLLONESHOT va a notificare l'evento una sola volta
                ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                ev.data.fd = client_fd;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
                    perror("epoll_ctl: client");
                    close(client_fd);
                }
            }
            // dati di un client
            else {
                // preparo il lavoro per il worker
                RequestContext* ctx = (RequestContext*)malloc(sizeof(RequestContext));
                ctx->client_fd = events[i].data.fd;
                ctx->epoll_fd = epoll_fd;
                ctx->db = db;
                // sottometto il lavoro al pool
                threadpool_add_job(pool, handle_http_request, ctx);
            }
        }
    }

cleanup_epoll:
    close(epoll_fd);
cleanup_socket:
    close(server_fd);
cleanup_db:
    threadpool_destroy(pool);
    db_close(db);
    printf("Server terminato.\n");
    exit(EXIT_FAILURE);
}