#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

#include "server_lib.h"
#include "server_clientservice.h"

#define DBFILE "server.db"

static sqlite3 *db;

void dbconnect()
{

    sqlite3_stmt *stmt;

    LOG("sqlite version %s", sqlite3_libversion());

    if (sqlite3_open(DBFILE, &db) != SQLITE_OK)
    {
        LOG("Connection to database %s failed: %s", DBFILE, sqlite3_errmsg(db));
        exit(1);
    }

    LOG("Connection to database %s established", DBFILE);

    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE id > ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, 0); // first question mark will be 0
    int step = sqlite3_step(stmt);
    if (step == SQLITE_ROW)
    {
        LOG("Number of users: %d", sqlite3_column_int(stmt, 0));
    }
}

void dbclose()
{
    sqlite3_close(db);
    LOG("Connection to database %s closed", DBFILE);
}

int checkUser(const char *login, const char *password)
{
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT id FROM users WHERE login = ? AND password = ?", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, login, strlen(login), NULL);
    sqlite3_bind_text(stmt, 2, password, strlen(password), NULL);
    return sqlite3_step(stmt) != SQLITE_ROW ? 0 : sqlite3_column_int(stmt, 0);
}

int getUser(const char *login)
{
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT id FROM users WHERE login = ?", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, login, strlen(login), NULL);
    return sqlite3_step(stmt) != SQLITE_ROW ? 0 : sqlite3_column_int(stmt, 0);
}

void readLogin(int id, char *login)
{
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT login FROM users WHERE id = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    if(sqlite3_step(stmt) == SQLITE_ROW) strcpy(login, (char *)sqlite3_column_text(stmt, 0));
}

void storeMessage(int from, int to, const char *message)
{
    time_t date = time(0);
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "INSERT INTO messages (date, sender, recipient, data, status) VALUES (?, ?, ?, ?, ?)", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, date);
    sqlite3_bind_int(stmt, 2, from);
    sqlite3_bind_int(stmt, 3, to);
    sqlite3_bind_text(stmt, 4, message, strlen(message), NULL);
    sqlite3_bind_int(stmt, 5, 0);
    sqlite3_step(stmt);
}

int getMessage(int to)
{
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT id FROM messages WHERE recipient = ? AND status = 0 ORDER BY date limit 1", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, to);
    return sqlite3_step(stmt) != SQLITE_ROW ? 0 : sqlite3_column_int(stmt, 0);
}

void readMessageAndMeta(int id, char *message, char *date, char *sender)
{
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT data, date, sender FROM messages WHERE id = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        strcpy(message, (char *)sqlite3_column_text(stmt, 0));
        time_t t = sqlite3_column_int(stmt, 1);
        strftime(date, 40, "%Y-%m-%d %H:%M:%S", localtime(&t));
        // time_t now = time(0);
        // strftime(date, 40, "%Y-%m-%d %H:%M:%S", localtime(&now));
        int id = sqlite3_column_int(stmt, 2);
        readLogin(id, sender);
    }
}

void markMessage(int id)
{
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "UPDATE messages SET status = 1 WHERE id = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
}