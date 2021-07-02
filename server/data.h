#ifndef DATA_H
#define DATA_H


#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <pthread.h>
#include <strings.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <getopt.h>

struct data_user_list
{
    long long uid;
    char login[32];
    struct data_user_list *next;
};

typedef struct  s_sql
{
    sqlite3 *db;
    pthread_mutex_t msg_mutex;
    long long last_msg_id;

}       t_sql;

typedef struct  s_param
{
    int     client_desc;
    t_sql   *s;
}       t_param;

struct thread_data
{
    char *login;
    int sock;
    t_sql *s;
};

void data_init(char *root_password, t_sql *s);
char *data_get_user_password(char *login, t_sql *s);
int data_validate_login(char *login);
int data_create_user(char *login, char *password, t_sql *s);
void data_create_session(char *login, t_sql *s);
void data_delete_session(const char *login, t_sql *s);

void data_new_message(char *kind, const char *login, const char *msg, t_sql *s);
long long data_last_message(t_sql *s);
void data_send_all(char *my_login, long long from, long long to, int sock, t_sql *s);
void data_send_history(int cnt, int sock, t_sql *s);
int data_get_user_list(struct data_user_list **st, t_sql *s);
void data_free_user_list(struct data_user_list *s);
int data_kick_user(long long uid, const char *reason, t_sql *s);
void data_clean();

void *connection_handler(void *);

#endif
