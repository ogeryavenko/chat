#include "data.h"
#include "bl.h"


static void _init_last_msg_id(t_sql *s)
{
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(s->db, "SELECT MAX(ROWID) FROM messages", -1, &stmt, 0);
    sqlite3_step(stmt);
    s->last_msg_id = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
}

void data_init(char *root_password, t_sql *s)
{
    s->msg_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    if(sqlite3_open("chat.sqlite", &s->db))
    {
        puts("Cannot open chat.sqlite");
        exit(1);
    }
    char *errmsg;
    if(sqlite3_exec(s->db, "CREATE TABLE IF NOT EXISTS users(login varchar(32) NOT NULL PRIMARY KEY, password varchar(32) NOT NULL)", 0, 0, &errmsg))
    {
        printf("Database error: %s\n", errmsg);
        sqlite3_close(s->db);
        exit(1);
    }
    sqlite3_exec(s->db, "CREATE TABLE IF NOT EXISTS messages(kind char(1), time int64 NOT NULL, login varchar(32), body text)", 0, 0, &errmsg);
    sqlite3_exec(s->db, "DROP TABLE IF EXISTS sessions", 0, 0, &errmsg);
    sqlite3_exec(s->db, "CREATE TABLE sessions(login varchar(32) NOT NULL)", 0, 0, &errmsg);
    sqlite3_exec(s->db, "DELETE FROM users WHERE login='root'", 0, 0, &errmsg);
    _init_last_msg_id(s);
    data_create_user("root", root_password, s);
}

char *data_get_user_password(char *login, t_sql *s)
{
    pthread_mutex_lock(&s->msg_mutex);
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(s->db, "SELECT password FROM users WHERE login=?", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, login, strlen(login), SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    if(rc == SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&s->msg_mutex);
        return 0;
    }
    const char *password = (const char *) sqlite3_column_text(stmt, 0);
    char *t = malloc(strlen(password) + 1);
    strcpy(t, password);
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&s->msg_mutex);
    return t;
}

int data_validate_login(char *login)
{
    if(strlen(login) > 31 || strlen(login) < 2)
    {
        return 0;
    }
    for(char *i=login;*i;++i)
    {
        if(*i < ' ')
        {
            return 0;
        }
    }
    return 1;
}

int data_create_user(char *login, char *password, t_sql *s)
{
    sqlite3_stmt *stmt;
    pthread_mutex_lock(&s->msg_mutex);
    sqlite3_prepare_v2(s->db, "INSERT INTO users VALUES (?, ?)", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, login, strlen(login), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, strlen(password), SQLITE_STATIC);
    if(sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&s->msg_mutex);
        return 0;
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&s->msg_mutex);
    return 1;
}

void data_create_session(char *login, t_sql *s)
{
    sqlite3_stmt *stmt;
    pthread_mutex_lock(&s->msg_mutex);
    sqlite3_prepare_v2(s->db, "INSERT INTO sessions(login) VALUES (?)", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, login, strlen(login), SQLITE_STATIC);
    if(sqlite3_step(stmt) != SQLITE_DONE)
    {
        puts("Failed to create session");
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&s->msg_mutex);
}

void data_delete_session(const char *login, t_sql *s)
{
    sqlite3_stmt *stmt;
    // DEBUG_LOCK
    pthread_mutex_lock(&s->msg_mutex);
    sqlite3_prepare_v2(s->db, "DELETE FROM sessions WHERE ROWID=(SELECT MIN(ROWID) FROM sessions WHERE login=?)", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, login, strlen(login), SQLITE_STATIC);
    if(sqlite3_step(stmt) != SQLITE_DONE)
    {
        puts("Failed to delete session");
    }
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&s->msg_mutex);
}

void data_new_message(char *kind, const char *login, const char *msg, t_sql *s)
{
    struct timeval tv;
    pthread_mutex_lock(&s->msg_mutex);
    gettimeofday(&tv, 0);
    long long time_encoded = ((long long)tv.tv_sec << 32) + tv.tv_usec;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(s->db, "INSERT INTO messages(kind, time, login, body) VALUES (?, ?, ?, ?)", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, kind, 1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, time_encoded);
    sqlite3_bind_text(stmt, 3, login, strlen(login), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, msg, strlen(msg), SQLITE_STATIC);
    if(sqlite3_step(stmt) != SQLITE_DONE)
    {
        puts(sqlite3_errmsg(s->db));
    }
    sqlite3_finalize(stmt);
    s->last_msg_id = sqlite3_last_insert_rowid(s->db);
    pthread_mutex_unlock(&s->msg_mutex);
   
}

long long data_last_message(t_sql *s)
{
    long long t = s->last_msg_id;
    return t;
}

void data_send_all(char *my_login, long long from, long long to, int sock, t_sql *s)
{
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(s->db, "SELECT * FROM messages WHERE ROWID>? AND ROWID<=?", -1, &stmt, 0);
    sqlite3_bind_int64(stmt, 1, from);
    sqlite3_bind_int64(stmt, 2, to);
    int rc;
    while((rc = sqlite3_step(stmt)) != SQLITE_DONE)
    {
        const char *kind = (const char *) sqlite3_column_text(stmt, 0);
        long long t = sqlite3_column_int64(stmt, 1);
        const char *login = (const char *) sqlite3_column_text(stmt, 2);
        const char *body = (const char *) sqlite3_column_text(stmt, 3);
        struct timeval tv;
        tv.tv_usec = t << 32 >> 32;
        tv.tv_sec = t >> 32;
        if(kind[0] == 'k')
        {
            if(strcmp(login, my_login))
                continue;
            else
            {
                my_login[0] = 0;
                data_delete_session(login, s);
            }
        }
        message_send(kind[0], tv, login, body, sock);
    }
    sqlite3_finalize(stmt);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
}

void data_send_history(int cnt, int sock, t_sql *s)
{
    sqlite3_stmt *stmt;
    if(cnt > 500)
        cnt = 500;
    sqlite3_prepare_v2(s->db, "SELECT * FROM (SELECT *, ROWID FROM messages WHERE kind='r' ORDER BY ROWID DESC LIMIT ?) ORDER BY ROWID ASC", -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, cnt);
    int rc;
    while((rc = sqlite3_step(stmt)) != SQLITE_DONE)
    {
        long long t = sqlite3_column_int64(stmt, 1);
        const char *login = (const char *) sqlite3_column_text(stmt, 2);
        const char *body = (const char *) sqlite3_column_text(stmt, 3);
        struct timeval tv;
        tv.tv_usec = t << 32 >> 32;
        tv.tv_sec = t >> 32;
        message_send('h', tv, login, body, sock);
    }
    sqlite3_finalize(stmt);
}

int data_get_user_list(struct data_user_list **st, t_sql *sql)
{
    struct data_user_list *s = 0;
    sqlite3_stmt *stmt;
    pthread_mutex_lock(&sql->msg_mutex);
    sqlite3_prepare_v2(sql->db, "SELECT users.ROWID, users.login FROM users INNER JOIN sessions ON users.login=sessions.login", -1, &stmt, 0);
    int len = 0;
    while(sqlite3_step(stmt) != SQLITE_DONE)
    {
        ++len;
        struct data_user_list *t = malloc(sizeof(struct data_user_list));
        t->uid = sqlite3_column_int64(stmt, 0);
        strcpy(t->login, (const char *) sqlite3_column_text(stmt, 1));
        t->next = s;
        s = t;
    }
    sqlite3_finalize(stmt);
    *st = s;
    pthread_mutex_unlock(&sql->msg_mutex);
    return len;
}

void data_free_user_list(struct data_user_list *s)
{
    while(s)
    {
        struct data_user_list *t = s->next;
        free(s);
        s = t;
    }
}

int data_kick_user(long long uid, const char *reason, t_sql *s)
{
    pthread_mutex_lock(&s->msg_mutex);
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(s->db, "SELECT login FROM users WHERE ROWID=? AND login IN (SELECT login FROM sessions)", -1, &stmt, 0);
    sqlite3_bind_int64(stmt, 1, uid);
    int rc = sqlite3_step(stmt);
    if(rc == SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&s->msg_mutex);
        return 0;
    }
    const char *login = (const char *) sqlite3_column_text(stmt, 0);
    pthread_mutex_unlock(&s->msg_mutex);
    data_new_message("k", login, reason, s);
    char buf[512];
    strcpy(buf, login);
    strcat(buf, " kicked");
    if(*reason)
    {
        strcat(buf, " (reason: ");
        strncat(buf, reason, 256);
        strcat(buf, ")");
    }
    sqlite3_finalize(stmt);
    data_new_message("m", "", buf, s);
    return 1;
}

void data_clean()
{
    unlink("chat.sqlite");
}
