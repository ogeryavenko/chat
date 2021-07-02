#ifndef WEB_H
#define WEB_H

#define STATUS_OK 0
#define STATUS_UNKNOWN_TYPE 1
#define STATUS_LOGIN_REQUIRED 2
#define STATUS_AUTH_ERROR 3
#define STATUS_SIGNUP_ERROR 4
#define STATUS_ACCESS_DENIED 5
#define STATUS_INVALID_MESSAGE 6
#define STATUS_NO_SUCH_USER 7

#define MESSAGE_BUF_SIZE (1 << 17)

#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <sys/socket.h>

struct web_line
{
    char *data;
    unsigned length;
};

struct web_message
{
    struct web_line *lines;
    unsigned line_count;
    char type;
};

int web_recv_packet(char *buf, int sock);

void web_free(struct web_message *s);
struct web_message *web_decode(char *buf, size_t length);
unsigned web_encode(struct web_message *msg, char **buf);
int web_get_int(struct web_message *msg, unsigned line_no);
char *web_get_str(struct web_message *msg, unsigned line_no);
unsigned web_get_len(struct web_message *msg, unsigned line_no);
unsigned web_get_line_count(struct web_message *msg);
char web_get_type(struct web_message *msg);
void web_get_timeval(struct web_message *msg, unsigned line_no, struct timeval *timestamp);
struct web_message *web_create(char type, unsigned line_count);
void web_set_int(struct web_message *msg, unsigned line_no, int value);
void web_set_blob(struct web_message *msg, unsigned line_no, const char *value, unsigned len);
void web_set_str(struct web_message *msg, unsigned line_no, const char *value);
void web_set_timeval(struct web_message *msg, unsigned line_no, struct timeval *value);

#endif
