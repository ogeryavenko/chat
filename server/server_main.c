#define _GNU_SOURCE

#include "bl.h"
#include "data.h"
#include "web.h"


void parse_opts(int argc, char **argv, int *port, char **root_password)
{
    const char *HELP_STR = "Usage: chat_server [-p PORT] [-r ROOT_PASSWORD]. Default port is 1337. Default root password is 12345.";

    struct option opts_list[] = {
        {"port", required_argument, NULL, 'p'},
        {"root-password", required_argument, NULL, 'r'},
        {"help", no_argument, NULL, 'h'},
        {"clean", no_argument, NULL, 'c'},
        {0, 0, 0,                               0}
    };
    char opts_string[] = "p:r:hc";
    int option_index = 0;
    while (1) {
        int res = getopt_long(argc, argv, opts_string, opts_list, &option_index);
        if (res == -1) {
            return;
        }
        switch (res) {
            case 'p':
                sscanf(optarg, "%d", port);
                break;
            case 'r':
                if(strlen(optarg) >= 32 || strlen(optarg) < 2)
                {
                    puts("Password length must be between 2 and 31.");
                    exit(1);
                }
                *root_password = optarg;
                break;
            case 'h':
                puts(HELP_STR);
                exit(0);
            case 'c':
                data_clean();
                break;
            default:
                puts("");
                puts(HELP_STR);
                exit(1);
        }
    }
}

void printerr(const char *s) {
    write(2, s, strlen(s));
}


int main(int argc, char **argv)
{
    t_sql *s = (t_sql*)malloc(sizeof(t_sql));
    if (!s)
        return -1;
    bzero(s, sizeof(t_sql));

    if (argc != 2) {
        printerr("uchat_server: error args\n");
        exit(1);
    }

    int desc;
    int port = 5000;
    char *root_password = "12345";
    parse_opts(argc, argv, &port, &root_password);
    struct sockaddr_in addr;
    desc = socket(AF_INET , SOCK_STREAM , 0);
    if(desc == -1)
    {
        printf("Failed to create socket\n");
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if(bind(desc, (struct sockaddr*) &addr, sizeof(addr)) < 0)
    {
        printf("Failed to bind to the port %d\n", errno);
    }
    listen(desc, 32);
    puts("Initializing...");
    data_init(root_password, s);
    puts("Waiting for clients");
    socklen_t c = sizeof(struct sockaddr_in);
    int client_desc;
    struct sockaddr_in client_addr;
    while(1)
    {
        client_desc = accept(desc, (struct sockaddr*) &client_addr, &c);
        if(client_desc < 0)
        {
            puts("Failed to accept a connection");
            sqlite3_sleep(100);
            continue;
        }
        puts("Connection accepted");
        pthread_t handler;

        t_param *param = (t_param*)malloc(sizeof(t_param));
        if (!param)
            return -1;
        param->client_desc = client_desc;

        param->s = s;

        if(pthread_create(&handler, 0, connection_handler, param) < 0)
        {
            printf("Failed to create thread %d\n", errno);
        }
        pthread_detach(handler);
    }
    return 0;
}

void *message_watcher(void *param)
{
    int sock = ((struct thread_data *)param)->sock;
    char *login = ((struct thread_data *)param)->login;
    t_sql *s = ((struct thread_data *)param)->s;
    long long cur = data_last_message(s);
    while(1)
    {
        pthread_testcancel();
        sqlite3_sleep(100);
        pthread_testcancel();
        long long t = data_last_message(s);
        if(t > cur)
        {
            data_send_all(login, cur, t, sock, s);
            cur = t;
        }
    }
    return param;
}

void *connection_handler(void *param)
{
    puts("Client thread created");
    int sock = ((t_param*)param)->client_desc;
    t_sql *s = (t_sql*)malloc(sizeof(t_sql));
    if (!s)
        return NULL;
    s = ((t_param*)param)->s;
    free(param);
    char *buf = malloc(MESSAGE_BUF_SIZE);
    char *login = 0;
    pthread_t watcher = 0;
    signal(SIGPIPE, SIG_IGN);
    while(1)
    {
        int read = web_recv_packet(buf, sock);

        if(read <= 0)
        {
            shutdown(sock, 2);
            puts("Client thread finished");
            break; // done reading
        }
        struct web_message *msg = web_decode(buf, read);
        if(!msg)
        {
            message_send_status(STATUS_INVALID_MESSAGE, sock);
            shutdown(sock, 2);
            puts("Broken message");
            break;
        }
        //CONTROLER
        switch(web_get_type(msg))
        {
            case 'i':  // login
                if(login)
                {
                    if(watcher)
                    {
                        pthread_cancel(watcher);
                        pthread_join(watcher, 0);
                        watcher = 0;
                    }
                    message_do_logout(login, s);
                    free(login);
                }
                login = message_login(msg, sock, s);
                if(login)
                {
                    printf("%s logged in.\n", login);
                    struct thread_data data;
                    data.login = login;
                    data.sock = sock;
                    data.s = s;
                    pthread_create(&watcher, 0, message_watcher, &data);
                }
                break;
            case 'o':  // logout
                if(watcher)
                {
                    pthread_cancel(watcher);
                    pthread_join(watcher, 0);
                    watcher = 0;
                }
                message_logout(login, sock, s);
                free(login);
                login = 0;
                break;
            case 'r':
                message_receive(login, msg, sock, s);
                break;
            case 'h':
                message_history(login, msg, sock, s);
                break;
            case 'l':
                message_list(login, sock, s);
                break;
            case 'k':
                message_kick(login, msg, sock, s);
                break;
            default:
                message_send_status(STATUS_UNKNOWN_TYPE, sock);
        }
        web_free(msg);
    }
    if(login)
    {
        if(watcher)
        {
            pthread_cancel(watcher);
            pthread_join(watcher, 0);
            watcher = 0;
        }
        if(*login)
            message_do_logout(login, s);
        free(login);
    }
    free(buf);
    return 0;
}
