#include <stdlib.h>
#include <stdio.h>

#include <sqlite3.h>

#include "clod.h"

struct CLOD_Sqlite3Store {
    struct CLOD_Store iface;
    sqlite3 *db;
    sqlite3_stmt *statement;
};

void sqlite3_store_save_section(struct CLOD_Store *self, struct CLOD_Section *section) {
    
}

void sqlite3_store_close(struct CLOD_Store *store) {
    struct CLOD_Sqlite3Store *self = (struct CLOD_Sqlite3Store*) store;
    
    sqlite3_close(self->db);
    free(self);
}

struct CLOD_Store *CLOD_open_sqlite3(char *path) {
    int error;

    struct CLOD_Sqlite3Store *self = malloc(sizeof(struct CLOD_Sqlite3Store));

    self->iface.save_section = &sqlite3_store_save_section;
    self->iface.close = &sqlite3_store_close;

    error = sqlite3_open(path, &self->db);
    if (error != SQLITE_OK) {
        fprintf(stderr, sqlite3_errmsg(self->db));
        sqlite3_close(self->db);
        free(self);
        return NULL;
    }

    sqlite3_prepare_v2(self->db, "select count(data) from FullData", -1, &self->statement, NULL);

        printf("%d LOD sections in database \n", sqlite3_column_int(self->statement, 0));

    sqlite3_finalize(self->statement);

    return &self->iface;
}

struct CLOD_PostgresStore {
    struct CLOD_Store iface;
};
