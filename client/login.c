#include "login.h"
#include "data.h"
#include "messages.h"


void *login_thread(void *param)
{
    ((struct login_info *)param)->t->sock = 0;
    char *res = message_connect(((struct login_info *)param)->ip, ((struct login_info *)param)->iport, ((struct login_info *)param)->t);
    if(!res)
        res = message_do_login(((struct login_info *)param)->login, ((struct login_info *)param)->password, ((struct login_info *)param)->t->sock, ((struct login_info *)param)->t);
    if(res)
    {
        gtk_label_set_text(GTK_LABEL(((struct login_info *)param)->t->statusLabel), res);
        message_disconnect(((struct login_info *)param)->t->sock);
    }
    else
    {
    	myCSS();
        init_chat_window(((struct login_info *)param)->login, ((struct login_info *)param)->t);
        ((struct login_info *)param)->t->logged_in = 1;
        free(param);
        return param;
    }
    free(param);
    gtk_widget_set_sensitive(((struct login_info *)param)->t->loginButton, 1);
    return param;
}

void do_login(GtkWidget *widget, t_test *t)
{
    (void) widget;
    if(!gtk_widget_get_sensitive(t->loginButton))
        return;
    const gchar *ip, *port, *login, *password;
    login = gtk_entry_get_text(GTK_ENTRY(t->loginEntry));
    if(!login || !*login)
    {
        gtk_widget_grab_focus(t->loginEntry);
        return;
    }
    password = gtk_entry_get_text(GTK_ENTRY(t->passwordEntry));
    if(!password || !*password)
    {
        gtk_widget_grab_focus(t->passwordEntry);
        return;
    }
    ip = gtk_entry_get_text(GTK_ENTRY(t->ipEntry));
    if(!ip || !*ip)
    {
        gtk_widget_grab_focus(t->ipEntry);
        return;
    }
    port = gtk_entry_get_text(GTK_ENTRY(t->portEntry));
    if(!port || !*port)
    {
        gtk_widget_grab_focus(t->portEntry);
        return;
    }
    int iport;
    if(!sscanf(port, "%d", &iport))
    {
        gtk_label_set_text(GTK_LABEL(t->statusLabel), "Invalid port");
        return;
    }
    gtk_widget_set_sensitive(t->loginButton, 0);
    struct login_info *li = malloc(sizeof(struct login_info));
    li->ip = ip;
    li->iport = iport;
    li->login = login;
    li->password = password;
    li->t = t;
    pthread_create(&li->t->loginner, 0, login_thread, (void *)li);
}

gboolean check_login(void *t)
{
    if(((t_test*)t)->logged_in)
    {
        gtk_widget_hide(((t_test*)t)->loginWindow);
        gtk_widget_show_all(((t_test*)t)->chatWindow);
        ((t_test*)t)->logged_in = 0;
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

void init_login_window(t_test *t)
{
    GtkBuilder *builder = gtk_builder_new_from_file("client/src/login.glade");

    t->loginWindow = GTK_WIDGET(gtk_builder_get_object(builder,"loginWindow"));
    g_signal_connect(t->loginWindow,"destroy", G_CALLBACK(gtk_main_quit),NULL);

    t->loginEntry = GTK_WIDGET(gtk_builder_get_object(builder,"loginEntry"));
    t->passwordEntry = GTK_WIDGET(gtk_builder_get_object(builder,"passwordEntry"));
    g_signal_connect(G_OBJECT(t->loginEntry),"activate", G_CALLBACK(do_login),t);
    g_signal_connect(G_OBJECT(t->passwordEntry),"activate", G_CALLBACK(do_login),t);
    t->ipEntry = GTK_WIDGET(gtk_builder_get_object(builder,"ipEntry"));
    t->portEntry = GTK_WIDGET(gtk_builder_get_object(builder,"portEntry"));
    g_signal_connect(G_OBJECT(t->ipEntry),"activate", G_CALLBACK(do_login),t);
    g_signal_connect(G_OBJECT(t->portEntry),"activate", G_CALLBACK(do_login),t);

    t->statusLabel = GTK_WIDGET(gtk_builder_get_object(builder,"statusLabel"));
    t->loginButton = GTK_WIDGET(gtk_builder_get_object(builder,"loginButton"));
    g_signal_connect(G_OBJECT(t->loginButton),"clicked", G_CALLBACK(do_login),t);
    t->logged_in = 0;
    g_timeout_add(50, check_login, t);
}
