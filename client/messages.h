#ifndef MESSAGES_H
#define MESSAGES_H

#include <sys/time.h>

char *message_connect(const char *ip, int port, t_test *t);
char *message_do_login(const char *login, const char *password, int sock, t_test *t);
void message_do_logout(int sock, t_test *t);
void message_send(const char *msg, int sock);
int message_receive(struct timeval *time, char **author, char **body, int sock, t_test *t);
void message_request_history(int cnt, int sock);
void message_request_list(int sock);
void message_kick_user(int uid, const char *reason, int sock);
void message_disconnect(int sock);

#endif
