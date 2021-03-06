#ifndef __SERVER_SQLITE_H
#define __SERVER_SQLITE_H

void dbconnect();
void dbclose();
int checkUser(const char *, const char *);
int getUser(const char *);
void readLogin(int, char *);

int getMessage(int);
void storeMessageAndMeta(int, int, int, const char *, const char *);
void readMessageAndMeta(int, char *, char *, char *);
void markMessage(int);

int getMigration();
void readMigration(int, char *);
void markMigration(int);

#endif