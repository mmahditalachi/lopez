#ifndef CHAT_H
#define CHAT_H

struct Message
{
    char sender[50];
    char receiver[50];
    char message[100];
    struct Message *next;
};

struct User
{
    char username[50];
    char datetime[50];
    struct Message *chatHistory;
    struct User *next;
    struct sockaddr_in clientAddr;
    int sockID;
    int len;
};

void addMessage(struct Message **history, const char *username, const char *message);
void addUser(struct User **head, const char *username, const char *datetime);
struct User *findUserByName(struct User *userList, char username[]);
void setUserSocketId(struct User *head, char *username, int socketId);
struct User *findUserBySocketId(int socketId);
char *createUserChat(struct User *userList);
char *getOnlineUserList(int currentClientSocketId);
#endif // CHAT_H