#ifndef DATA_H
# define DATA_H

#define REQUEST_HISTORY 10

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "web.h"

typedef struct s_test {
    GtkWidget *chatWindow;
    GtkWidget *togglebutton;
    GtkWidget *togglebutton2;
    bool togglebutton1State;
    bool togglebutton2State;
    GtkWidget *chatLabel;
    GtkWidget *sendEntry;
    GtkWidget *sendButton;
    GtkWidget *createChatLabel;
    GtkWidget *writeMessageLabel;
    GtkWidget *createEnt;
    GtkWidget *createButton;
    GtkWidget *writeEnt;
    GtkWidget *writeButton;
    GtkWidget *messagesTreeView;
    GtkAdjustment *vAdjust;
    GtkScrolledWindow *scrolledWindow;
    GtkListStore *messagesListStore;
    GtkWidget *statusLabel;
    int sock;
    char message_buf[MESSAGE_BUF_SIZE];
    GtkWidget *loginEntry;
    GtkWidget *passwordEntry;
    GtkWidget *ipEntry;
    GtkWidget *portEntry;
    GtkWidget *loginButton;
    GtkWidget *loginWindow;
    pthread_t loginner;
    int logged_in;
}              t_test;

void init_chat_window();
void myCSS();


#endif
