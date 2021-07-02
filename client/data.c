#define _GNU_SOURCE

// #define REQUEST_HISTORY 10

#include "data.h"
#include "messages.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <gtk/gtk.h>

// GtkWidget *chatWindow;
// GtkWidget *sendEntry, *sendButton;
// GtkWidget *statusLabel;
// GtkWidget *messagesTreeView;
// GtkAdjustment *vAdjust;
// GtkScrolledWindow *scrolledWindow;
// GtkListStore *messagesListStore;

// pthread_t watcher;
// const char *HELP_STR = "\\l: list of active users, \\k ID REASON: kick (root only)";


void sleep_ms(int milliseconds)
{
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void add_list_entry(const char *t, const char *a, const char *m, int sleep, t_test *test)
{
    GtkTreeIter iter;
    gtk_list_store_append(GTK_LIST_STORE(test->messagesListStore), &iter);
    gtk_list_store_set(GTK_LIST_STORE(test->messagesListStore), &iter, 0, t, 1, a, 2, m, -1);
    if(sleep)
        sleep_ms(100);
    gtk_adjustment_set_value(test->vAdjust, gtk_adjustment_get_upper(test->vAdjust) - gtk_adjustment_get_page_size(test->vAdjust));
}

void do_send(GtkWidget *widget, t_test *t)
{
    (void)widget;
    const char *HELP_STR = "\\l: list of active users, \\k ID REASON: kick (root only)";

    if(!gtk_widget_get_sensitive(t->sendButton))
        return;
    gtk_label_set_text(GTK_LABEL(t->statusLabel), "");
    const gchar *message;
    message = gtk_entry_get_text(GTK_ENTRY(t->sendEntry));
    if(!message || !*message)
        return;
    if(message[0] == '\\' && message[1])
    {
        if(message[1] == '?' && (!message[2] || message[2] == ' '))
        {
            gtk_label_set_text(GTK_LABEL(t->statusLabel), HELP_STR);
            gtk_entry_set_text(GTK_ENTRY(t->sendEntry), "");
            return;
        }
        if(message[1] == 'l' && (!message[2] || message[2] == ' '))
        {
            message_request_list(t->sock);
            gtk_entry_set_text(GTK_ENTRY(t->sendEntry), "");
            return;
        }
        if(message[1] == 'k' && message[2] == ' ')
        {
            int uid;
            char num[16];
            int i = 3, j = 0;
            while(message[i] == ' ')
                ++i;
            while(j < 15 && message[i] != ' ' && message[i])
                num[j++] = message[i++];
            if(j == 15)
            {
                gtk_label_set_text(GTK_LABEL(t->statusLabel), "Usage: \\k ID REASON");
                gtk_entry_set_text(GTK_ENTRY(t->sendEntry), "");
                return;
            }
            num[j] = 0;
            sscanf(num, "%d", &uid);
            while(message[i] == ' ')
                ++i;
            message_kick_user(uid, &message[i], t->sock);
            gtk_entry_set_text(GTK_ENTRY(t->sendEntry), "");
            return;
        }
        gtk_label_set_text(GTK_LABEL(t->statusLabel), "Unknown command, type \\? for help");
        gtk_entry_set_text(GTK_ENTRY(t->sendEntry), "");
        return;
    }
    char *m = malloc(strlen(message) + 1);
    strcpy(m, message);
    gtk_entry_set_text(GTK_ENTRY(t->sendEntry), "");
    message_send(m, t->sock);
    free(m);
}

void *watcher_thread(void *t)
{
    struct timeval tv;
    struct tm *nowtm;
    char *author, *body;
    char timebuf[64];
    message_request_history(REQUEST_HISTORY, ((t_test*)t)->sock);
    while(1)
    {
        int k = message_receive(&tv, &author, &body, ((t_test*)t)->sock, (t_test*)t);
        if(k < 0)
        {
            gtk_label_set_text(GTK_LABEL(((t_test*)t)->statusLabel), "Disconnected, please restart the client");
            gtk_widget_set_sensitive(((t_test*)t)->sendButton, 0);
            break;
        }
        if(k == 0)
            continue;
        if(!author)
        {
            gtk_label_set_text(GTK_LABEL(((t_test*)t)->statusLabel), body);
            continue;
        }
        if(tv.tv_sec)
        {
            nowtm = localtime(&tv.tv_sec);
            strftime(timebuf, 64, "[%d.%m.%Y %H:%M:%S]", nowtm);
        }
        else
        {
            *timebuf = 0;
        }
        add_list_entry(timebuf, author, body, k != '?', (t_test*)t);
        free(author);
        free(body);
    }
    return 0;
}


void room_chat_chosen(GtkWidget *widget)
{
	printf("%s chat chosen\n", gtk_button_get_label(GTK_BUTTON(widget)));
}

void 	read_chatrooms(GtkBuilder *builder)
{
	GtkStyleContext	*context;
	GtkWidget		*button[1000];
	GtkWidget		*grid1;
	FILE 			*f1;
	char			tmp[1024];
	int				row;

	row = 0;
	grid1 = GTK_WIDGET(gtk_builder_get_object(builder, "grid1"));
	if (!(f1 = fopen("client/src/list.txt", "r+")))
	{
		printf("FILE NOT FOUND\n");
		return;
	}

	GtkCssProvider *provider;
	provider = gtk_css_provider_new ();

	while (fgets(tmp, 1024, f1) != NULL)
	{
		tmp[strlen(tmp)-1] = 0;
		gtk_grid_insert_row(GTK_GRID(grid1),row);
		button[row] = gtk_button_new_with_label(tmp);
		context = gtk_widget_get_style_context(button[row]);
		gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
		gtk_style_context_add_class(context, "list_button");

		gtk_widget_set_size_request(GTK_WIDGET(button[row]),250,1);
		gtk_grid_attach(GTK_GRID(grid1),button[row],1,row,1,1);
		g_signal_connect (button[row], "clicked", G_CALLBACK (room_chat_chosen), NULL);
		row++;
	}
	fclose(f1);
}

void on_togglebuttonState_toggled(GtkWidget *widget, t_test *t) {
	gboolean togglebutton_state;

	togglebutton_state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    if (togglebutton_state && !t->togglebutton2State) {
        gtk_button_set_label(GTK_BUTTON(t->togglebutton), "Dark");
        t->togglebutton1State = true;
    }
    else if (togglebutton_state && t->togglebutton2State) {
        gtk_button_set_label(GTK_BUTTON(t->togglebutton), "Темная");
        t->togglebutton1State = true;
    }
    else if (!togglebutton_state && t->togglebutton2State) {
        gtk_button_set_label(GTK_BUTTON(t->togglebutton), "Светлая");
        t->togglebutton1State = false;
    }
    else if (!togglebutton_state && !t->togglebutton2State) {
        gtk_button_set_label(GTK_BUTTON(t->togglebutton), "Light");
        t->togglebutton1State = false;
    }
	if (togglebutton_state) {
        GtkCssProvider *provider = gtk_css_provider_new();
	    GtkStyleContext	*context = gtk_widget_get_style_context(t->chatWindow);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
        gtk_style_context_add_class(context, "dark_theme");
	}
	else {
        GtkCssProvider *provider = gtk_css_provider_new();
	    GtkStyleContext	*context = gtk_widget_get_style_context(t->chatWindow);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
        gtk_style_context_remove_class(context, "dark_theme");
	}
}

void on_togglebuttonState_toggled_2(GtkWidget *widget, t_test *t) {
	gboolean togglebutton_state;

	togglebutton_state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	if (togglebutton_state && !t->togglebutton1State) {
        gtk_button_set_label(GTK_BUTTON(t->togglebutton), "Светлая");
        t->togglebutton2State = true;
	}
    else if (togglebutton_state && t->togglebutton1State) {
        gtk_button_set_label(GTK_BUTTON(t->togglebutton), "Темная");
        t->togglebutton2State = true;
    }
    else if (!togglebutton_state && t->togglebutton1State) {
        gtk_button_set_label(GTK_BUTTON(t->togglebutton), "Light");
        t->togglebutton2State = false;
    }
    else if (!togglebutton_state && !t->togglebutton1State) {
        gtk_button_set_label(GTK_BUTTON(t->togglebutton), "Dark");
        t->togglebutton2State = false;
    }
	if (togglebutton_state) {
        gtk_button_set_label(GTK_BUTTON(t->togglebutton2), "English");
        // gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(t->stackSwitcher), "English");
        gtk_label_set_text(GTK_LABEL(t->chatLabel), "Для подсказки введите \\?");
        gtk_entry_set_placeholder_text(GTK_ENTRY(t->sendEntry), "Введите сообщение");
        gtk_button_set_label(GTK_BUTTON(t->sendButton), "Отправить");
        gtk_label_set_text(GTK_LABEL(t->createChatLabel), "Создать чат:");
        gtk_entry_set_placeholder_text(GTK_ENTRY(t->createEnt), "Название чата");
        gtk_button_set_label(GTK_BUTTON(t->createButton), "Создать");
        gtk_label_set_text(GTK_LABEL(t->writeMessageLabel), "Добавить друга:");
        gtk_entry_set_placeholder_text(GTK_ENTRY(t->writeEnt), "Имя друга");
        gtk_button_set_label(GTK_BUTTON(t->writeButton), "Добавить");
	}
    else {
        gtk_button_set_label(GTK_BUTTON(t->togglebutton2), "Russian");
        gtk_label_set_text(GTK_LABEL(t->chatLabel), "For help type \\?");
        gtk_entry_set_placeholder_text(GTK_ENTRY(t->sendEntry), "Type a message");
        gtk_button_set_label(GTK_BUTTON(t->sendButton), "Send");
        gtk_label_set_text(GTK_LABEL(t->createChatLabel), "Create a chat:");
        gtk_entry_set_placeholder_text(GTK_ENTRY(t->createEnt), "Chat name");
        gtk_button_set_label(GTK_BUTTON(t->createButton), "Create");
        gtk_label_set_text(GTK_LABEL(t->writeMessageLabel), "Add a friend:");
        gtk_entry_set_placeholder_text(GTK_ENTRY(t->writeEnt), "Friend name");
        gtk_button_set_label(GTK_BUTTON(t->writeButton), "Add");
    }
}

void init_chat_window(char *login, t_test *t)
{
    pthread_t watcher;
    
    GtkBuilder *builder = gtk_builder_new_from_file("client/src/chat.glade");

    t->chatWindow = GTK_WIDGET(gtk_builder_get_object(builder,"chatWindow"));
    char buf[100] = "Login: ";
    strcat(buf, login);
    gtk_window_set_title(GTK_WINDOW(t->chatWindow), buf);
    g_signal_connect(t->chatWindow,"destroy", G_CALLBACK(gtk_main_quit),NULL);
    t->sendEntry = GTK_WIDGET(gtk_builder_get_object(builder,"sendEntry"));
    t->sendButton = GTK_WIDGET(gtk_builder_get_object(builder,"sendButton"));
    g_signal_connect(G_OBJECT(t->sendEntry),"activate", G_CALLBACK(do_send),t);
    g_signal_connect(G_OBJECT(t->sendButton),"clicked", G_CALLBACK(do_send),t);
    t->statusLabel = GTK_WIDGET(gtk_builder_get_object(builder,"statusLabel"));
    t->messagesTreeView = GTK_WIDGET(gtk_builder_get_object(builder,"messagesTreeView"));
    t->messagesListStore = GTK_LIST_STORE(gtk_builder_get_object(builder,"messagesListStore"));
    t->scrolledWindow = GTK_SCROLLED_WINDOW(gtk_builder_get_object(builder,"scrolledWindow"));
    t->vAdjust = gtk_scrolled_window_get_vadjustment(t->scrolledWindow);

    t->togglebutton1State = false;
    t->togglebutton2State = false;
    GtkWidget *togglebutton = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton"));
	g_signal_connect(togglebutton, "toggled", G_CALLBACK(on_togglebuttonState_toggled), t);
    t->togglebutton = togglebutton;
    GtkWidget *togglebutton2 = GTK_WIDGET(gtk_builder_get_object(builder, "togglebutton2"));
	g_signal_connect(togglebutton2, "toggled", G_CALLBACK(on_togglebuttonState_toggled_2), t);
    t->togglebutton2 = togglebutton2;
    GtkWidget *chatLabel = GTK_WIDGET(gtk_builder_get_object(builder, "chatLabel"));
    t->chatLabel = chatLabel;
    GtkWidget *createChatLabel = GTK_WIDGET(gtk_builder_get_object(builder, "createChatLabel"));
    t->createChatLabel = createChatLabel;
    GtkWidget *createEnt = GTK_WIDGET(gtk_builder_get_object(builder, "create_ent"));
    t->createEnt = createEnt;
    GtkWidget *createButton = GTK_WIDGET(gtk_builder_get_object(builder, "create_btn"));
    t->createButton = createButton;
    GtkWidget *writeMessageLabel = GTK_WIDGET(gtk_builder_get_object(builder, "writeMessageLabel"));
    t->writeMessageLabel = writeMessageLabel;
    GtkWidget *writeEnt = GTK_WIDGET(gtk_builder_get_object(builder, "write_ent"));
    t->writeEnt = writeEnt;
    GtkWidget *writeButton = GTK_WIDGET(gtk_builder_get_object(builder, "write_btn"));
    t->writeButton = writeButton;

	read_chatrooms(builder);

    pthread_create(&watcher, 0, watcher_thread, t);
}
