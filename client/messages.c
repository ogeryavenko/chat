#define _GNU_SOURCE

#include "data.h"
#include "web.h"


static void _send(struct web_message *p, int sock)
{
    char *buf;
    unsigned len = web_encode(p, &buf);
    web_free(p);
    send(sock, buf, len, 0);
    free(buf);
}

static int resolve_host(const char *host)
{
    int s;
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(host, "http", &hints, &result))
        return 0;
    s = ((struct sockaddr_in *) result->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(result);
    return s;
}

char *message_connect(const char *ip, int port, t_test *t)
{
    struct sockaddr_in addr;
    t->sock = socket(AF_INET, SOCK_STREAM, 0);
    if(t->sock < 0)
    {
        return "Cannot create socket";
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(!(addr.sin_addr.s_addr = resolve_host(ip)))
    {
        return "Invalid host";
    }
    if(connect(t->sock, (struct sockaddr*)&addr,  sizeof(struct sockaddr)) < 0)
    {
        shutdown(t->sock, 2);
        return "Unable to connect";
    }
    return 0;
}

char *message_do_login(const char *login, const char *password, int sock, t_test *test)
{
    struct web_message *p = web_create('i', 2);
    web_set_str(p, 0, login);
    web_set_str(p, 1, password);
    _send(p, sock);
    unsigned len = web_recv_packet(test->message_buf, sock);
    if(len <= 0)
    {
        return "Connection failed";
    }
    p = web_decode(test->message_buf, len);
    if(!p || web_get_type(p) != 's' || web_get_line_count(p) < 1 || web_get_len(p, 0) != 4)
    {
        return "Unknown error";
    }
    int t = web_get_int(p, 0);
    web_free(p);
    switch(t)
    {
        case STATUS_OK:
            return 0;
        case STATUS_SIGNUP_ERROR:
            return "Signup error";
        case STATUS_AUTH_ERROR:
            return "Incorrect password";
        default:
            return "Unknown error";
    }
}

void message_send(const char *msg, int sock)
{
    struct web_message *p = web_create('r', 1);
    web_set_str(p, 0, msg);
    _send(p, sock);
}

int message_receive(struct timeval *time, char **author, char **body, int sock, t_test *t)
{
    int len = web_recv_packet(t->message_buf, sock);
    if(len <= 0)
        return -1;
    struct web_message *p = web_decode(t->message_buf, len);
    if(!p)
        return -1;
    int tp = web_get_type(p);
    if(tp == 'r' || tp == 'h')
    {
        web_get_timeval(p, 0, time);
        char *s1 = web_get_str(p, 1);
        char *s2 = web_get_str(p, 2);
        *author = malloc(strlen(s1) + 1);
        strcpy(*author, s1);
        *body = malloc(strlen(s2) + 1);
        strcpy(*body, s2);
    }
    else if(tp == 'm')
    {
        web_get_timeval(p, 0, time);
        char *s1 = web_get_str(p, 1);
        *author = malloc(1);
        **author = 0;
        *body = malloc(strlen(s1) + 1);
        strcpy(*body, s1);
    }
    else if(tp == 'l')
    {
        int cnt = web_get_line_count(p) / 2;
        int len = 20 + cnt * 50;
        *body = malloc(len);
        strcpy(*body, "User list: ");
        for(int i=0;i<cnt;++i)
        {
            int uid = web_get_int(p, i * 2);
            char *login = web_get_str(p, i * 2 + 1);
            if(strlen(login) > 32)
                continue;
            sprintf(t->message_buf, "%s%s (%d)", (i?", ":""), login, uid);
            strcat(*body, t->message_buf);
        }
        *author = malloc(1);
        **author = 0;
        time->tv_sec = 0;
    }
    else if(tp == 's' && web_get_line_count(p) == 1 && web_get_int(p, 0) == STATUS_ACCESS_DENIED)
    {
        *author = 0;
        *body = "Access denied";
    }
    else if(tp == 's' && web_get_line_count(p) == 1 && web_get_int(p, 0) == STATUS_NO_SUCH_USER)
    {
        *author = 0;
        *body = "No such user";
    }
    else if(tp == 'k')
    {
        char *reason = web_get_str(p, 0);
        *author = malloc(1);
        **author = 0;
        *body = malloc(300);
        strcpy(*body, "You have been kicked (reason: ");
        strncat(*body, reason, 256);
        strcat(*body, ")");
        time->tv_sec = 0;
    }
    else
    {
        tp = 0;
    }
    web_free(p);
    return tp;
}

void message_request_history(int cnt, int sock)
{
    struct web_message *p = web_create('h', 1);
    web_set_int(p, 0, cnt);
    _send(p, sock);
}

void message_request_list(int sock)
{
    struct web_message *p = web_create('l', 0);
    _send(p, sock);
}

void message_kick_user(int uid, const char *reason, int sock)
{
    struct web_message *p = web_create('k', 2);
    web_set_int(p, 0, uid);
    web_set_str(p, 1, reason);
    _send(p, sock);
}

void message_disconnect(int sock)
{
    shutdown(sock, 2);
}
