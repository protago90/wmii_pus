#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

#include "server_lib.h"
#include "server_clientservice.h"

#define DBFILE "server.db"

static sqlite3 *db;

void dbconnect() {
    sqlite3_stmt *stmt;
    LOG("sqlite version %s", sqlite3_libversion());
    if (sqlite3_open(DBFILE, &db) != SQLITE_OK) {
        LOG("Connection to database %s failed: %s", DBFILE, sqlite3_errmsg(db));
        exit(1);
    }
    LOG("Connection to database %s established", DBFILE);
    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE id > ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, 0); // first question mark will be 0
    if (sqlite3_step(stmt) == SQLITE_ROW) LOG("Number of users: %d", sqlite3_column_int(stmt, 0));
}

void dbclose() {
    sqlite3_close(db);
    LOG("Connection to database %s closed", DBFILE);
}

int checkUser(const char *login, const char *password) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT id FROM users WHERE login = ? AND password = ?", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, login, strlen(login), NULL);
    sqlite3_bind_text(stmt, 2, password, strlen(password), NULL);
    return sqlite3_step(stmt) != SQLITE_ROW ? 0 : sqlite3_column_int(stmt, 0);
}

int getUser(const char *login) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT id FROM users WHERE login = ?", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, login, strlen(login), NULL);
    return sqlite3_step(stmt) != SQLITE_ROW ? 0 : sqlite3_column_int(stmt, 0);
}

void readLogin(int id, char *login) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT login FROM users WHERE id = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) strcpy(login, (char *)sqlite3_column_text(stmt, 0));
}

void storeMessageAndMeta(int from, int to, int datemode, const char *message, const char *host) {
    int date;
    char migration[MAXINPUT];
    if (datemode == 0) {
        date = time(0);
        snprintf(migration, sizeof(migration), "%d;%s;%d;%d;%s", date, host, from, to, message);
    } else {
        date = datemode;
        strcpy(migration, "done");
    }
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "INSERT INTO messages (date, host, sender, recipient, data, status, migration) VALUES (?, ?, ?, ?, ?, 0, ?)", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, date);
    sqlite3_bind_text(stmt, 2, host, strlen(host), NULL);
    sqlite3_bind_int(stmt, 3, from);
    sqlite3_bind_int(stmt, 4, to);
    sqlite3_bind_text(stmt, 5, message, strlen(message), NULL);
    sqlite3_bind_text(stmt, 6, migration, strlen(migration), NULL);
    sqlite3_step(stmt);
}

int getMessage(int to) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT id FROM messages WHERE recipient = ? AND status = 0 ORDER BY date limit 1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, to);
    return sqlite3_step(stmt) != SQLITE_ROW ? 0 : sqlite3_column_int(stmt, 0);
}

void readMessageAndMeta(int id, char *message, char *date, char *sender) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT data, date, sender FROM messages WHERE id = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        strcpy(message, (char *)sqlite3_column_text(stmt, 0));
        time_t t = sqlite3_column_int(stmt, 1);
        strftime(date, 40, "%Y-%m-%d %H:%M:%S", localtime(&t));
        int id = sqlite3_column_int(stmt, 2);
        readLogin(id, sender);
    }
}

void markMessage(int id) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "UPDATE messages SET status = 1 WHERE id = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
}

int getMigration() {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT id FROM messages WHERE migration != 'done' ORDER BY date LIMIT 1", -1, &stmt, NULL);
    return sqlite3_step(stmt) != SQLITE_ROW ? 0 : sqlite3_column_int(stmt, 0);
}

void readMigration(int id, char *migration) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT migration FROM messages WHERE id = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        strcpy(migration, (char *)sqlite3_column_text(stmt, 0));
    }
}

void markMigration(int id) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "UPDATE messages SET migration = 'done' WHERE id = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
}
