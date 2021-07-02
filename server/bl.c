#include "bl.h"
#include "data.h"
#include "web.h"

// #include <string.h>
// #include <sys/socket.h>
// #include <stdio.h>

static void _send(struct web_message *p, int sock)
{
    char *buf;
    unsigned len = web_encode(p, &buf);
    web_free(p);
    send(sock, buf, len, 0);
    free(buf);
}

void message_send_status(int status, int sock)
{
    struct web_message *p = web_create('s', 1);
    web_set_int(p, 0, status);
    _send(p, sock);
}

void message_send(char kind, struct timeval time, const char *login, const char *body, int sock)
{
    if(kind == 'r' || kind == 'h')
    {
        struct web_message *p = web_create(kind, 3);
        web_set_timeval(p, 0, &time);
        web_set_str(p, 1, login);
        web_set_str(p, 2, body);
        _send(p, sock);
    }
    if(kind == 'm')
    {
        struct web_message *p = web_create('m', 2);
        web_set_timeval(p, 0, &time);
        web_set_str(p, 1, body);
        _send(p, sock);
    }
    if(kind == 'k')
    {
        struct web_message *p = web_create('k', 1);
        web_set_str(p, 0, body);
        _send(p, sock);
        shutdown(sock, 2);
    }
}

// ----------  Buissness Logics
char *message_login(struct web_message *m, int sock, t_sql *s)
{
    if(web_get_line_count(m) < 2)
    {
        message_send_status(STATUS_INVALID_MESSAGE, sock);
        return 0;
    }
    char *login = web_get_str(m, 0);
    char *password = web_get_str(m, 1);
    if(!data_validate_login(login))
    {
        message_send_status(STATUS_SIGNUP_ERROR, sock);
        return 0;
    }
    char *db_password = data_get_user_password(login, s);
    if(!db_password)
    {
        if(strlen(password) < 2 || strlen(password) > 31)
        {
            message_send_status(STATUS_SIGNUP_ERROR, sock);
            return 0;
        }
        data_create_user(login, password, s);
        printf("Created user %s\n", login);
    }
    else if(strcmp(password, db_password))
    {
        free(db_password);
        message_send_status(STATUS_AUTH_ERROR, sock);
        return 0;
    }
    free(db_password);
    message_send_status(STATUS_OK, sock);
    data_create_session(login, s);

    char buf[64];
    strcpy(buf, login);
    strcat(buf, " logged in");
    data_new_message("m", "", buf, s);
    char *t = malloc(strlen(login) + 1);
    strcpy(t, login);
    return t;
}

void message_do_logout(char *login, t_sql *s)
{
    data_delete_session(login, s);
    printf("%s logged out\n", login);
    char buf[64];
    strcpy(buf, login);
    strcat(buf, " logged out");
    data_new_message("m", "", buf, s);
}

void message_logout(char *login, int sock, t_sql *s)
{
    if(login)
    {
        message_do_logout(login, s);
        message_send_status(STATUS_OK, sock);
    }
    else
    {
        message_send_status(STATUS_LOGIN_REQUIRED, sock);
    }
}

void message_receive(char *login, struct web_message *m, int sock, t_sql *s)
{
    if(web_get_line_count(m) < 1)
    {
        message_send_status(STATUS_INVALID_MESSAGE, sock);
        return;
    }
    if(!login)
    {
        message_send_status(STATUS_LOGIN_REQUIRED, sock);
        return;
    }
    char *line = web_get_str(m, 0);
    printf("Message from %s: %s\n", login, line);
    data_new_message("r", login, line, s);
}

void message_history(char *login, struct web_message *m, int sock, t_sql *s)
{
    if(web_get_len(m, 0) != 4)
    {
        message_send_status(STATUS_INVALID_MESSAGE, sock);
        return;
    }
    if(!login)
    {
        message_send_status(STATUS_LOGIN_REQUIRED, sock);
        return;
    }
    int cnt = web_get_int(m, 0);
    data_send_history(cnt, sock, s);
    printf("Sent %d old messages to %s\n", cnt, login);
}

void message_list(char *login, int sock, t_sql *sql)
{
    if(!login)
    {
        message_send_status(STATUS_LOGIN_REQUIRED, sock);
        return;
    }
    struct data_user_list *s, *t;
    int len = data_get_user_list(&s, sql);
    t = s;
    struct web_message *p = web_create('l', 2 * len);
    for(int i=0;s;++i,s=s->next)
    {
        web_set_int(p, 2 * i, (int) s->uid);
        web_set_str(p, 2 * i + 1, s->login);
    }
    data_free_user_list(t);
    _send(p, sock);
    printf("Sent user list to %s\n", login);
}

void message_kick(char *login, struct web_message *m, int sock, t_sql *s)
{
    if(web_get_line_count(m) < 2 || web_get_len(m, 0) != 4)
    {
        message_send_status(STATUS_INVALID_MESSAGE, sock);
        return;
    }
    if(!login)
    {
        message_send_status(STATUS_LOGIN_REQUIRED, sock);
        return;
    }
    if(strcmp(login, "root"))
    {
        printf("%s tried to kick\n", login);
        message_send_status(STATUS_ACCESS_DENIED, sock);
        return;
    }
    int uid = web_get_int(m, 0);
    char *reason = web_get_str(m, 1);
    if(data_kick_user(uid, reason, s))
    {
        printf("Kicked user %d: %s\n", uid, reason);
    }
    else
    {
        message_send_status(STATUS_NO_SUCH_USER, sock);
        printf("Tried to kick %d: %s\n", uid, reason);
    }
}
