#ifndef BL_H
#define BL_H

#include "data.h"
#include "web.h"

void message_send_status(int status, int sock);
char *message_login(struct web_message *m, int sock, t_sql *s);
void message_do_logout(char *login, t_sql *s);
void message_logout(char *login, int sock, t_sql *s);
void message_receive(char *login, struct web_message *m, int sock, t_sql *s);
void message_send(char kind, struct timeval time, const char *login, const char *body, int sock);
void message_history(char *login, struct web_message *m, int sock, t_sql *s);
void message_list(char *login, int sock, t_sql *s);
void message_kick(char *login, struct web_message *m, int sock, t_sql *s);

#endif
