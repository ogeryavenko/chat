CC = clang
FLAGS= -std=c11 -Wall -Wextra -Werror -Wpedantic -pthread -I ./web

CHAT_SERVER		=	uchat_server
CHAT_CLIENT		=	uchat


WEB_FILES		=	web.c

CLIENT_FILES	=	data.c\
					client_main.c\
					login.c\
					messages.c

SERVER_FILES	=	bl.c\
            	 	data.c\
            	 	server_main.c

PATH_WEB := ./web
PATH_CLIENT := ./client
PATH_SERVER := ./server
OBJ_CLIENT_PATH := ./obj_client
OBJ_SERVER_PATH := ./obj_server
OBJ_CLIENT := $(CLIENT_FILES:.c=.o) $(WEB_FILES:.c=.o)
OBJ_SERVER := $(SERVER_FILES:.c=.o) $(WEB_FILES:.c=.o)
OBJ_CLIENT := $(addprefix $(OBJ_CLIENT_PATH)/,$(OBJ_CLIENT))
OBJ_SERVER := $(addprefix $(OBJ_SERVER_PATH)/,$(OBJ_SERVER))
CLIENT_FILES := $(addprefix $(PATH_CLIENT)/, $(CLIENT_FILES))
SERVER_FILES := $(addprefix $(PATH_SERVER)/, $(SERVER_FILES))

all: $(CHAT_SERVER) $(CHAT_CLIENT)

$(OBJ_SERVER_PATH):
	mkdir -p $(OBJ_SERVER_PATH)
$(CHAT_SERVER): $(OBJ_SERVER)
	$(CC) $(OBJ_SERVER) -lsqlite3 -o $(CHAT_SERVER)
$(OBJ_SERVER_PATH)/%.o: $(PATH_SERVER)/%.c | $(OBJ_SERVER_PATH)
	$(CC) $(FLAGS) -c $< -o $@
$(OBJ_SERVER_PATH)/%.o: $(PATH_WEB)/%.c | $(OBJ_SERVER_PATH)
	$(CC) $(FLAGS) -c $< -o $@

$(OBJ_CLIENT_PATH):
	mkdir -p $(OBJ_CLIENT_PATH)
$(CHAT_CLIENT): $(OBJ_CLIENT)
	$(CC) $(OBJ_CLIENT) `pkg-config --cflags --libs gtk+-3.0` -o $(CHAT_CLIENT)
$(OBJ_CLIENT_PATH)/%.o: $(PATH_CLIENT)/%.c | $(OBJ_CLIENT_PATH)
	$(CC) $(FLAGS) `pkg-config --cflags gtk+-3.0` -c $< -o $@
$(OBJ_CLIENT_PATH)/%.o: $(PATH_WEB)/%.c | $(OBJ_CLIENT_PATH)
	$(CC) $(FLAGS) -c $< -o $@

uninstall:
	rm -rf $(OBJ_SERVER)
	rm -rf $(OBJ_SERVER_PATH)
	rm -rf $(OBJ_CLIENT)
	rm -rf $(OBJ_CLIENT_PATH)

clean: uninstall
	rm -rf $(CHAT_CLIENT)
	rm -rf $(CHAT_SERVER)

reinstall: clean all
