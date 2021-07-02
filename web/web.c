#include "web.h"

static unsigned read_num(char *buf)
{
    unsigned a = *(unsigned*)buf;
    return (a << 24) | ((a & 0x0000ff00) << 8) | ((a & 0x00ff0000) >> 8) | (a >> 24);
}

static void write_num(unsigned a, char *buf)
{
    *(unsigned*)buf = (a << 24) | ((a & 0x0000ff00) << 8) | ((a & 0x00ff0000) >> 8) | (a >> 24);
}

static int recv_bytes(char *buf, int sock, int len)
{
    while(len)
    {
        int t = recv(sock, buf, len, 0);
        if(t <= 0)
            return t;
        len -= t;
        buf += t;
    }
    return 1;
}

int web_recv_packet(char *buf, int sock)
{
    int r = recv_bytes(buf, sock, 5);
    if(r <= 0)
        return 0;
    int len = read_num(buf + 1);
    if(len > MESSAGE_BUF_SIZE - 5)
        return 0;
    r = recv_bytes(buf + 5, sock, len);
    if(r <= 0)
        return 0;
    return len + 5;
}

void web_free(struct web_message *s)
{
    if(!s)
        return;
    if(!s->lines)
    {
        free(s);
        return;
    }
    for(unsigned i=0;i<s->line_count;++i)
    {
        if(s->lines[i].data)
            free(s->lines[i].data);
    }
    free(s->lines);
    free(s);
}

struct web_message *web_decode(char *buf, size_t length)
{
    struct web_message *res;
    if(length < 5)
        return 0;
    char *end = buf + length;
    res = malloc(sizeof(struct web_message));
    res->type = buf[0];
    unsigned size = read_num(++buf);
    res->line_count = 0;
    buf += 4;
    for(char *d=buf;d!=buf+size;res->line_count++)
    {
        if(d > buf+size-4)
        {
            free(res);
            return 0;
        }
        d += read_num(d) + 4;
    }
    res->lines = calloc(res->line_count, sizeof(struct web_line));
    for(unsigned i=0;i<res->line_count;++i)
    {
        if(end - buf < 4)
        {
            web_free(res);
            return 0;
        }
        unsigned len = read_num(buf);
        buf += 4;
        if(end - buf < len)
        {
            web_free(res);
            return 0;
        }
        res->lines[i].length = len;
        res->lines[i].data = malloc(len + 1);
        memcpy(res->lines[i].data, buf, len);
        res->lines[i].data[len] = 0;
        buf += len;
    }
    return res;
}

unsigned web_encode(struct web_message *msg, char **buf)
{
    unsigned length = 5;
    for(unsigned i=0;i<msg->line_count;++i)
    {
        length += 4 + msg->lines[i].length;
    }
    *buf = malloc(length);
    char *data = *buf;
    *data++ = msg->type;
    write_num(length - 5, data);
    data += 4;
    for(unsigned i=0;i<msg->line_count;++i)
    {
        write_num(msg->lines[i].length, data);
        data += 4;
        if(msg->lines[i].length)
            memcpy(data, msg->lines[i].data, msg->lines[i].length);
        data += msg->lines[i].length;
    }
    return length;
}

int web_get_int(struct web_message *msg, unsigned line_no)
{
    if(line_no >= msg->line_count || msg->lines[line_no].length != 4)
        return 0;
    return read_num(msg->lines[line_no].data);
}

char *web_get_str(struct web_message *msg, unsigned line_no)
{
    if(line_no >= msg->line_count)
        return 0;
    return msg->lines[line_no].data;
}

unsigned web_get_len(struct web_message *msg, unsigned line_no)
{
    if(line_no >= msg->line_count)
        return 0;
    return msg->lines[line_no].length;
}

unsigned web_get_line_count(struct web_message *msg)
{
    return msg->line_count;
}

char web_get_type(struct web_message *msg)
{
    return msg->type;
}

void web_get_timeval(struct web_message *msg, unsigned line_no, struct timeval *timestamp)
{
    if(line_no >= msg->line_count || msg->lines[line_no].length != 8)
    {
        timestamp->tv_sec = timestamp->tv_usec = 0;
        return;
    }
    timestamp->tv_sec = read_num(msg->lines[line_no].data);
    timestamp->tv_usec = read_num(msg->lines[line_no].data + 4);
}

struct web_message *web_create(char type, unsigned line_count)
{
    struct web_message *res = malloc(sizeof(struct web_message));
    res->type = type;
    res->line_count = line_count;
    res->lines = calloc(line_count, sizeof(struct web_line));
    return res;
}

void web_set_int(struct web_message *msg, unsigned line_no, int value)
{
    if(line_no >= msg->line_count)
        return;
    if(msg->lines[line_no].data)
        free(msg->lines[line_no].data);
    msg->lines[line_no].length = 4;
    msg->lines[line_no].data = malloc(5);
    write_num(value, msg->lines[line_no].data);
}

void web_set_blob(struct web_message *msg, unsigned line_no, const char *value, unsigned len)
{
    if(line_no >= msg->line_count)
        return;
    if(msg->lines[line_no].data)
        free(msg->lines[line_no].data);
    msg->lines[line_no].length = len;
    msg->lines[line_no].data = malloc(len + 1);
    if(len)
        memcpy(msg->lines[line_no].data, value, len);
    msg->lines[line_no].data[len] = 0;
}

void web_set_str(struct web_message *msg, unsigned line_no, const char *value)
{
    web_set_blob(msg, line_no, value, strlen(value));
}

void web_set_timeval(struct web_message *msg, unsigned line_no, struct timeval *value)
{
    if(line_no >= msg->line_count)
        return;
    if(msg->lines[line_no].data)
        free(msg->lines[line_no].data);
    msg->lines[line_no].length = 8;
    msg->lines[line_no].data = malloc(9);
    write_num(value->tv_sec, msg->lines[line_no].data);
    write_num(value->tv_usec, msg->lines[line_no].data + 4);
}
