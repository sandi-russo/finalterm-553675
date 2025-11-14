#ifndef DB_HANDLER_H
#define DB_HANDLER_H

#include "common.h"

sqlite3* db_init();
void db_close(sqlite3* db);
char* db_get_all_users(sqlite3* db);
char* db_get_user(sqlite3* db, int id);
int db_create_user(sqlite3* db, const char* name);
int db_delete_user(sqlite3* db, int id);

#endif