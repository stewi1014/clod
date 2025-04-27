/*
#include <stdlib.h>
#include <stdio.h>

#include <sqlite3.h>

#include <lod.h>

struct LOD_Sqlite3Store {
    struct LOD_Store iface;
    sqlite3 *db;
    sqlite3_stmt *statement;
};

void sqlite3_store_save_section(struct LOD_Store *self, struct LOD_Section *section) {
    
}

void sqlite3_store_close(struct LOD_Store *store) {
    struct LOD_Sqlite3Store *self = (struct LOD_Sqlite3Store*) store;
    
    sqlite3_close(self->db);
    free(self);
}

struct LOD_Store *LOD_open_sqlite3(char *path) {
    int error;

    struct LOD_Sqlite3Store *self = malloc(sizeof(struct LOD_Sqlite3Store));

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

struct LOD_PostgresStore {
    struct LOD_Store iface;
};

*/