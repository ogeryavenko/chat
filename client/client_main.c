#include "login.h"
#include <stdio.h>
#include <gtk/gtk.h>

void printerr(const char *s) {
    write(2, s, strlen(s));
}


void myCSS()
{
	GtkCssProvider *provider;
	GdkDisplay *display;
	GdkScreen *screen;

	provider = gtk_css_provider_new ();
	display = gdk_display_get_default ();
	screen = gdk_display_get_default_screen (display);
	gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	const gchar *myCssFile = "client/src/widgets.css";
	GError *error = 0;

	gtk_css_provider_load_from_file(provider, g_file_new_for_path(myCssFile), &error);
	g_object_unref (provider);
}

int main(int argc,char *argv[]){

	t_test *t = (t_test*)malloc(sizeof(t_test));
    if (!t)
        return -1;
    bzero(t, sizeof(t_test));
    if (argc != 3) {
        printerr("uchat_server: error args\n");
        printerr("example: ./uchat ip port\n");
        exit(1);
    }
    gtk_init(&argc, &argv);
    myCSS();
    init_login_window(t);
    gtk_widget_show_all(t->loginWindow);
    gtk_main();
    return 0;
}
