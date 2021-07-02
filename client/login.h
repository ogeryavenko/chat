#ifndef LOGIN_H
#define LOGIN_H

#include <gtk/gtk.h>
#include <strings.h>
#include <stdio.h>

#include "data.h"

struct login_info
{
    const char *ip;
    const char *login;
    const char *password;
    int iport;
    t_test *t;
};

void init_login_window(t_test *t);


#endif
