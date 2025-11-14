#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <pthread.h>
#include <sqlite3.h>
#define PORT 8080
#define MAX_EVENTS 10
#define BUFFER_SIZE 2048
#define DB_FILE "rest_api.db"
#define NUM_WORKERS 4

#endif