#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gc.h>

#include <sqlite3.h>

#include "dh.h"

extern unsigned char __0010_sqlite_createInitialDataTables_sql[];
extern unsigned int __0010_sqlite_createInitialDataTables_sql_len;

extern unsigned char __0020_sqlite_createFullDataSourceV2Tables_sql[];
extern unsigned int __0020_sqlite_createFullDataSourceV2Tables_sql_len;

extern unsigned char __0030_sqlite_changeTableJournaling_sql[];
extern unsigned int __0030_sqlite_changeTableJournaling_sql_len;

extern unsigned char __0031_sqlite_useSqliteWalJournaling_sql[];
extern unsigned int __0031_sqlite_useSqliteWalJournaling_sql_len;

extern unsigned char __0040_sqlite_removeRenderCache_sql[];
extern unsigned int __0040_sqlite_removeRenderCache_sql_len;

extern unsigned char __0050_sqlite_addApplyToParentIndex_sql[];
extern unsigned int __0050_sqlite_addApplyToParentIndex_sql_len;

extern unsigned char __0060_sqlite_createChunkHashTable_sql[];
extern unsigned int __0060_sqlite_createChunkHashTable_sql_len;

extern unsigned char __0070_sqlite_createBeaconBeamTable_sql[];
extern unsigned int __0070_sqlite_createBeaconBeamTable_sql_len;

extern unsigned char __0080_sqlite_addApplyToChildrenColumn_sql[];
extern unsigned int __0080_sqlite_addApplyToChildrenColumn_sql_len;

int run_migration(sqlite3 *db, char *name, const char *sql, int sql_size) {
    sqlite3_stmt *stmt;
    const char *tail;
    int err;

    err = sqlite3_prepare_v2(db, "select * from Schema where ScriptName=?",-1, &stmt, NULL);
    if (err != SQLITE_OK) {
        fprintf(stderr, "checking migration %s: (%d) %s\n", name, err, sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name, -1, NULL);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err == SQLITE_ROW) return 0;

    while (sql_size > 0) {
        err = sqlite3_prepare_v2(db, sql, sql_size, &stmt, &tail);
        if (err != SQLITE_OK) {
            fprintf(stderr, "preparing %s: (%d) %s\n", name, err, sqlite3_errmsg(db));
            return -1;
        }

        if (stmt != NULL) {
            err = sqlite3_step(stmt);
            if (err != SQLITE_DONE && err != SQLITE_ROW) {
                fprintf(stderr, "executing %s: (%d) %s\n", name, err, sqlite3_errmsg(db));
                return -1;
            }

            sqlite3_finalize(stmt);
        }

        sql_size -= (tail - sql);
        sql = tail;
    };

    err = sqlite3_prepare_v2(db, "insert into Schema (ScriptName) values(?)",-1, &stmt, NULL);
    if (err != SQLITE_OK) {
        fprintf(stderr, "checking migration %s: (%d) %s\n", name, err, sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name, -1, NULL);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err == SQLITE_ROW) return 0;

    return 0;
}

struct dh_db {
    sqlite3 *db;

    sqlite3_stmt *store;
};

struct dh_db *dh_db_open(char *path) {
    int err;
    struct dh_db *db;

    db = calloc(1, sizeof(struct dh_db));
    if (db == NULL) return NULL;

    err = sqlite3_open_v2(path, &db->db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (err != SQLITE_OK) {
        fprintf(stderr, "sqlite3_open: %s\n", sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        free(db);
        return NULL;
    }

    sqlite3_stmt *stmt;
    err = sqlite3_prepare_v2(db->db, "SELECT name FROM sqlite_master WHERE name='Schema'", -1, &stmt, NULL);
    if (err != SQLITE_OK) {
        fprintf(stderr, "sqlite3_prepare_v2: %s\n", sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        free(db);
        return NULL;
    }
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err != SQLITE_ROW) {
        err = sqlite3_prepare_v2(db->db,
            "CREATE TABLE Schema ( \n"
            "    SchemaVersionId INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, \n"
            "    ScriptName TEXT NOT NULL UNIQUE, \n"
            "    AppliedDateTime DATETIME NOT NULL default CURRENT_TIMESTAMP \n"
            ")",
            -1,&stmt, NULL
        );
        if (err != SQLITE_OK) {
            fprintf(stderr, "create Schema sql: %s\n", sqlite3_errmsg(db->db));
            sqlite3_close(db->db);
            free(db);
            return NULL;
        }

        err = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (err != SQLITE_DONE) {
            fprintf(stderr, "execute create Schema: %s\n", sqlite3_errmsg(db->db));
            sqlite3_close(db->db);
            free(db);
            return NULL;
        }
    }

    #define MIGRATION(name, sql, sql_len) \
    if (run_migration(db->db, name, (const char*)sql, sql_len)) \
        { sqlite3_close(db->db); free(db); return NULL; }

    MIGRATION(
        "sqlScripts/0010_sqlite_createInitialDataTables.sql",
        __0010_sqlite_createInitialDataTables_sql,
        __0010_sqlite_createInitialDataTables_sql_len
    );

    MIGRATION(
        "sqlScripts/0020_sqlite_createFullDataSourceV2Tables.sql",
        __0020_sqlite_createFullDataSourceV2Tables_sql,
        __0020_sqlite_createFullDataSourceV2Tables_sql_len
    );

    MIGRATION(
        "sqlScripts/0030_sqlite_changeTableJournaling.sql",
        __0030_sqlite_changeTableJournaling_sql,
        __0030_sqlite_changeTableJournaling_sql_len
    );

    MIGRATION(
        "sqlScripts/0031_sqlite_useSqliteWalJournaling.sql",
        __0031_sqlite_useSqliteWalJournaling_sql,
        __0031_sqlite_useSqliteWalJournaling_sql_len
    );

    MIGRATION(
        "sqlScripts/0040_sqlite_removeRenderCache.sql",
        __0040_sqlite_removeRenderCache_sql,
        __0040_sqlite_removeRenderCache_sql_len
    );

    MIGRATION(
        "sqlScripts/0050_sqlite_addApplyToParentIndex.sql",
        __0050_sqlite_addApplyToParentIndex_sql,
        __0050_sqlite_addApplyToParentIndex_sql_len
    );

    MIGRATION(
        "sqlScripts/0060_sqlite_createChunkHashTable.sql",
        __0060_sqlite_createChunkHashTable_sql,
        __0060_sqlite_createChunkHashTable_sql_len
    );

    MIGRATION(
        "sqlScripts/0070_sqlite_createBeaconBeamTable.sql",
        __0070_sqlite_createBeaconBeamTable_sql,
        __0070_sqlite_createBeaconBeamTable_sql_len
    );

    MIGRATION(
        "sqlScripts/0080_sqlite_addApplyToChildrenColumn.sql",
        __0080_sqlite_addApplyToChildrenColumn_sql,
        __0080_sqlite_addApplyToChildrenColumn_sql_len
    );


    char *store_sql =
        "insert into FullData ("
        "DetailLevel, "
        "PosX, "
        "PosZ, "
        "MinY, "
        "DataChecksum, "
        "Data, "
        "ColumnGenerationStep, "
        "ColumnWorldCompressionMode, "
        "Mapping, "
        "DataFormatVersion, "
        "CompressionMode, "
        "ApplyToParent, "
        "ApplyToChildren, "
        "LastModifiedUnixDateTime, "
        "CreatedUnixDateTime"
        ") values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

    err = sqlite3_prepare_v2(db->db, store_sql, strlen(store_sql)+1, &db->store, NULL);
    if (err != SQLITE_OK) {
        fprintf(stderr, "sqlite3_prepate_v2: %s\n", sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        free(db);
        return NULL;
    }

    return db;
}

void dh_db_close(struct dh_db *db) {
    int error;

    if (db == NULL) return;

    if (db->db != NULL){
        error = sqlite3_finalize(db->store);
        if (error != SQLITE_OK) {
            fprintf(stderr, "sqlite3_close: %s\n", sqlite3_errmsg(db->db));
        }
        db->store = NULL;

        error = sqlite3_close(db->db);
        if (error != SQLITE_OK) {
            fprintf(stderr, "sqlite3_close: %s\n", sqlite3_errmsg(db->db));
        }
        db->db = NULL;
    }

    free(db);

}

int dh_db_store(struct dh_db *db, struct dh_db_lod *lod) {
    if (db == NULL || lod == NULL) return -1;

    #define CHECK_ERROR(stmt) ({ if ((stmt) != SQLITE_OK) \
        fprintf(stderr, #stmt ": %s\n", sqlite3_errmsg(db->db)); })

    CHECK_ERROR(sqlite3_bind_int(db->store, 1, lod->detail_level));
    CHECK_ERROR(sqlite3_bind_int(db->store, 2, lod->pos_x));
    CHECK_ERROR(sqlite3_bind_int(db->store, 3, lod->pos_z));
    CHECK_ERROR(sqlite3_bind_int(db->store, 4, lod->min_y));
    CHECK_ERROR(sqlite3_bind_int(db->store, 5, lod->data_checksum));
    CHECK_ERROR(sqlite3_bind_blob(db->store, 6, lod->data, 0, NULL));
    CHECK_ERROR(sqlite3_bind_blob(db->store, 7, lod->column_generation_step, 0, NULL));
    CHECK_ERROR(sqlite3_bind_blob(db->store, 8, lod->column_world_compression_mode, 0, NULL));
    CHECK_ERROR(sqlite3_bind_blob(db->store, 9, lod->mapping, 0, NULL));
    CHECK_ERROR(sqlite3_bind_int(db->store, 10, lod->data_format_version));
    CHECK_ERROR(sqlite3_bind_int(db->store, 11, lod->compression_mode));
    CHECK_ERROR(sqlite3_bind_int(db->store, 12, lod->apply_to_parent));
    CHECK_ERROR(sqlite3_bind_int(db->store, 13, lod->apply_to_children));
    CHECK_ERROR(sqlite3_bind_int64(db->store, 14, lod->last_modified_time));
    CHECK_ERROR(sqlite3_bind_int64(db->store, 15, lod->created_time));

    int error;
    error = sqlite3_step(db->store);
    if (error != SQLITE_DONE) {
        fprintf(stderr, "sqlite3_step FullData: (%d) %s\n", error, sqlite3_errmsg(db->db));
        return -1;
    }

    error = sqlite3_reset(db->store);
    if (error != SQLITE_OK) {
        fprintf(stderr, "sqlite3_reset: (%d) %s\n", error, sqlite3_errmsg(db->db));
        return -1;
    }

    return 0;
}
