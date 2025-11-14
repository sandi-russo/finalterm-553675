#include "db_handler.h"

// funzione helper per eseguire SQL
static int db_exec(sqlite3* db, const char* sql) {
    char* err_msg = 0;
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    return 1;
}

// inizializza il database
sqlite3* db_init() {
    sqlite3* db;
    int rc = sqlite3_open(DB_FILE, &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }

    const char* sql = "CREATE TABLE IF NOT EXISTS users ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "name TEXT NOT NULL);";
    
    if (!db_exec(db, sql)) {
        sqlite3_close(db);
        return NULL;
    }
    
    printf("Database inizializzato e tabella 'users' pronta.\n");
    return db;
}

void db_close(sqlite3* db) {
    sqlite3_close(db);
}

// ottengo tutti gli utenti
char* db_get_all_users(sqlite3* db) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, name FROM users;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    char* json_result = (char*)calloc(BUFFER_SIZE * 2, sizeof(char));
    strcat(json_result, "[");
    
    int first_row = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first_row) {
            strcat(json_result, ",");
        }
        
        char user_json[512];
        sprintf(user_json, "{\"id\":%d, \"name\":\"%s\"}",
                sqlite3_column_int(stmt, 0),
                sqlite3_column_text(stmt, 1));
        
        strcat(json_result, user_json);
        first_row = 0;
    }
    
    strcat(json_result, "]");
    sqlite3_finalize(stmt);
    return json_result;
}

// ottengo un singolo utente
char* db_get_user(sqlite3* db, int id) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, name FROM users WHERE id = ?;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return NULL; // Errore
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        char* user_json = (char*)malloc(512);
        sprintf(user_json, "{\"id\":%d, \"name\":\"%s\"}",
                sqlite3_column_int(stmt, 0),
                sqlite3_column_text(stmt, 1));
        
        sqlite3_finalize(stmt);
        return user_json;
    }
    
    sqlite3_finalize(stmt);
    return NULL;
}

// creo un nuovo utente
int db_create_user(sqlite3* db, const char* name) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO users (name) VALUES (?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }

    char real_name[100];
    sscanf(name, "name=%s", real_name);

    sqlite3_bind_text(stmt, 1, real_name, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE);
}

// cancello un utente
int db_delete_user(sqlite3* db, int id) {
    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM users WHERE id = ?;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_DONE) {
        // controlla se una riga è stata effettivamente cancellata
        return (sqlite3_changes(db) > 0);
    }
    return 0;
}