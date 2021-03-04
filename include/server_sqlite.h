#ifndef __SERVER_SQLITE_H
#define __SERVER_SQLITE_H

void dbconnect();
void dbclose();
int checkUser(const char *, const char *);
int getUser(const char *);
int getMessage(int);
void storeMessage(int, int, const char *);
void readLogin(int, char *);
void readMessageAndMeta(int id, char *, char *, char *);
void markMessage(int);

#endif