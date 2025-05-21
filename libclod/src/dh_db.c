#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include "dh.h"
#include "generated/index.h"

int run_migration(sqlite3 *db, char *name, const char *sql, size_t sql_size) {
    sqlite3_stmt *stmt;
    const char *tail;

    int err = sqlite3_prepare_v2(db, "select * from Schema where ScriptName=?", -1, &stmt, nullptr);
    if (err != SQLITE_OK) {
        fprintf(stderr, "checking migration %s: (%d) %s\n", name, err, sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name, -1, nullptr);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err == SQLITE_ROW) return 0;

    while (sql_size > 0) {
        err = sqlite3_prepare_v2(db, sql, (int)sql_size, &stmt, &tail);
        if (err != SQLITE_OK) {
            fprintf(stderr, "preparing %s: (%d) %s\n", name, err, sqlite3_errmsg(db));
            return -1;
        }

        if (stmt != nullptr) {
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

    err = sqlite3_prepare_v2(db, "insert into Schema (ScriptName) values(?)", -1, &stmt, nullptr);
    if (err != SQLITE_OK) {
        fprintf(stderr, "checking migration %s: (%d) %s\n", name, err, sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name, -1, nullptr);
    err = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (err == SQLITE_ROW) return 0;

    return 0;
}

struct dh_db {
    sqlite3 *db;

    sqlite3_stmt *store;
};

struct dh_db *dh_db_open(const char *path) {
    struct dh_db* db = calloc(1, sizeof(struct dh_db));
    if (db == nullptr) return nullptr;

    int err = sqlite3_open_v2(path, &db->db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (err != SQLITE_OK) {
        fprintf(stderr, "sqlite3_open: %s\n", sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        free(db);
        return nullptr;
    }

    sqlite3_stmt *stmt;
    err = sqlite3_prepare_v2(db->db, "SELECT name FROM sqlite_master WHERE name='Schema'", -1, &stmt, nullptr);
    if (err != SQLITE_OK) {
        fprintf(stderr, "sqlite3_prepare_v2: %s\n", sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        free(db);
        return nullptr;
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
            -1,&stmt, nullptr
        );
        if (err != SQLITE_OK) {
            fprintf(stderr, "create Schema sql: %s\n", sqlite3_errmsg(db->db));
            sqlite3_close(db->db);
            free(db);
            return nullptr;
        }

        err = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (err != SQLITE_DONE) {
            fprintf(stderr, "execute create Schema: %s\n", sqlite3_errmsg(db->db));
            sqlite3_close(db->db);
            free(db);
            return nullptr;
        }
    }

    #define MIGRATION(name, sql, sql_len) \
    if (run_migration(db->db, name, (const char*)sql, sql_len)) \
        { sqlite3_close(db->db); free(db); return nullptr; }

    MIGRATION(
        "sqlScripts/0010-sqlite-createInitialDataTables.sql",
        dh_migrations__0010__sqlite__createInitialDataTables__sql,
        sizeof(dh_migrations__0010__sqlite__createInitialDataTables__sql)
    );

    MIGRATION(
        "sqlScripts/0020-sqlite-createFullDataSourceV2Tables.sql",
        dh_migrations__0020__sqlite__createFullDataSourceV2Tables__sql,
        sizeof(dh_migrations__0020__sqlite__createFullDataSourceV2Tables__sql)
    );

    MIGRATION(
        "sqlScripts/0030-sqlite-changeTableJournaling.sql",
        dh_migrations__0030__sqlite__changeTableJournaling__sql,
        sizeof(dh_migrations__0030__sqlite__changeTableJournaling__sql)
    );

    MIGRATION(
        "sqlScripts/0031-sqlite-useSqliteWalJournaling.sql",
        dh_migrations__0031__sqlite__useSqliteWalJournaling__sql,
        sizeof(dh_migrations__0031__sqlite__useSqliteWalJournaling__sql)
    );

    MIGRATION(
        "sqlScripts/0040-sqlite-removeRenderCache.sql",
        dh_migrations__0040__sqlite__removeRenderCache__sql,
        sizeof(dh_migrations__0040__sqlite__removeRenderCache__sql)
    );

    MIGRATION(
        "sqlScripts/0050-sqlite-addApplyToParentIndex.sql",
        dh_migrations__0050__sqlite__addApplyToParentIndex__sql,
        sizeof(dh_migrations__0050__sqlite__addApplyToParentIndex__sql)
    );

    MIGRATION(
        "sqlScripts/0060-sqlite-createChunkHashTable.sql",
        dh_migrations__0060__sqlite__createChunkHashTable__sql,
        sizeof(dh_migrations__0060__sqlite__createChunkHashTable__sql)
    );

    MIGRATION(
        "sqlScripts/0070-sqlite-createBeaconBeamTable.sql",
        dh_migrations__0070__sqlite__createBeaconBeamTable__sql,
        sizeof(dh_migrations__0070__sqlite__createBeaconBeamTable__sql)
    );

    MIGRATION(
        "sqlScripts/0080-sqlite-addApplyToChildrenColumn.sql",
        dh_migrations__0080__sqlite__addApplyToChildrenColumn__sql,
        sizeof(dh_migrations__0080__sqlite__addApplyToChildrenColumn__sql)
    );


    const char *store_sql =
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
        ") values (?,?,?,?,?,?,?,?,?,?,?,0,0,?,?)";

    err = sqlite3_prepare_v2(db->db, store_sql, (int)strlen(store_sql)+1, &db->store, nullptr);
    if (err != SQLITE_OK) {
        fprintf(stderr, "sqlite3_prepate_v2: %s\n", sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        free(db);
        return nullptr;
    }

    err = sqlite3_prepare_v2(db->db, "pragma journal_mode = OFF; PRAGMA synchronous = OFF; ", -1, &stmt, nullptr);
    if (err != SQLITE_OK) {
        fprintf(stderr, "sqlite3_prepate_v2: %s\n", sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        free(db);
        return nullptr;
    }

    do {
        err = sqlite3_step(stmt);
    } while (err == SQLITE_ROW);
    if (err != SQLITE_DONE) {
        fprintf(stderr, "sqlite3_step: %s\n", sqlite3_errmsg(db->db));
        sqlite3_close(db->db);
        free(db);
        return nullptr;
    }

    sqlite3_finalize(stmt);

    return db;
}

void dh_db_close(struct dh_db *db) {
    sqlite3_stmt *stmt;

    if (db == nullptr) return;

    if (db->db != nullptr){
        int err = sqlite3_prepare_v2(db->db, "pragma journal_mode = WAL; pragma synchronous = NORMAL; ", -1, &stmt, nullptr);
        if (err != SQLITE_OK) {
            fprintf(stderr, "sqlite3_prepare_v2: %s\n", sqlite3_errmsg(db->db));
        }

        do {
            err = sqlite3_step(stmt);
        } while (err == SQLITE_ROW);
        if (err != SQLITE_DONE) {
            fprintf(stderr, "sqlite3_step: %s\n", sqlite3_errmsg(db->db));
        }
        
        err = sqlite3_finalize(stmt);
        if (err != SQLITE_OK) {
            fprintf(stderr, "sqlite3_finalize: %s\n", sqlite3_errmsg(db->db));
        }

        err = sqlite3_finalize(db->store);
        if (err != SQLITE_OK) {
            fprintf(stderr, "sqlite3_finalize: %s\n", sqlite3_errmsg(db->db));
        }
        db->store = nullptr;

        err = sqlite3_close(db->db);
        if (err != SQLITE_OK) {
            fprintf(stderr, "sqlite3_close: %s\n", sqlite3_errmsg(db->db));
        }
        db->db = nullptr;
    }

    free(db);

}

int dh_db_store(const struct dh_db *db, struct dh_lod *lod) {
    if (db == nullptr || lod == nullptr) return -1;

    size_t mapping_len;
    char *mapping;
    const dh_result result = dh_lod_serialise_mapping(lod, &mapping, &mapping_len);
    if (result != DH_OK) {
        return -1;
    }

    #define check_error(stmt) ({ if ((stmt) != SQLITE_OK) \
        fprintf(stderr, #stmt ": %s\n", sqlite3_errmsg(db->db)); })

    switch (lod->compression_mode) {
    case DH_DATA_COMPRESSION_UNCOMPRESSED:
        check_error(sqlite3_bind_blob(db->store, 7, dh_constants__gen_step, sizeof(dh_constants__gen_step), SQLITE_STATIC));
        check_error(sqlite3_bind_blob(db->store, 8, dh_constants__compression_mode, sizeof(dh_constants__compression_mode), SQLITE_STATIC));
        break;
    case DH_DATA_COMPRESSION_LZ4:
        check_error(sqlite3_bind_blob(db->store, 7, dh_constants__gen_step__lz4, sizeof(dh_constants__gen_step__lz4), SQLITE_STATIC));
        check_error(sqlite3_bind_blob(db->store, 8, dh_constants__compression_mode__lz4, sizeof(dh_constants__compression_mode__lz4), SQLITE_STATIC));
        break;
    case DH_DATA_COMPRESSION_LZMA2:
        check_error(sqlite3_bind_blob(db->store, 7, dh_constants__gen_step__lzma, sizeof(dh_constants__gen_step__lzma), SQLITE_STATIC));
        check_error(sqlite3_bind_blob(db->store, 8, dh_constants__compression_mode__lzma, sizeof(dh_constants__compression_mode__lzma), SQLITE_STATIC));
        break;
    default:
        fprintf(stderr, "unknown LOD compression mode\n");
        return -1;
    }

    check_error(sqlite3_bind_int(db->store, 1, lod->mip_level));
    check_error(sqlite3_bind_int(db->store, 2, lod->x));
    check_error(sqlite3_bind_int(db->store, 3, lod->z));
    check_error(sqlite3_bind_int(db->store, 4, lod->min_y));
    check_error(sqlite3_bind_int(db->store, 5, 0));
    check_error(sqlite3_bind_blob(db->store, 6, lod->lod_arr, lod->lod_len, SQLITE_STATIC));
    check_error(sqlite3_bind_blob(db->store, 9, mapping, mapping_len, SQLITE_STATIC));
    check_error(sqlite3_bind_int(db->store, 10, 1));
    check_error(sqlite3_bind_int(db->store, 11, lod->compression_mode)); // TODO; add compression
    check_error(sqlite3_bind_int64(db->store, 12, 0));
    check_error(sqlite3_bind_int64(db->store, 13, 0));

    #undef check_error

    int error = sqlite3_step(db->store);
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
