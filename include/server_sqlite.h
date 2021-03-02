#ifndef __SERVER_SQLITE_H
#define __SERVER_SQLITE_H

void dbconnect();
void dbclose();
int checkUser(const char *, const char *);
int getUser(const char *);
void storeMessage(int, int, const char *);

#endif