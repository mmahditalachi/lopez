#ifndef CHAT_H
#define CHAT_H

struct Message
{
    char sender[50];
    char receiver[50];
    char message[250];
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

void addMessage(struct Message **history, const char *username, const char *receiver, const char *message);
void addUser(struct User **head, const char *username, const char *datetime);
struct User *findUserByName(char username[]);
void setUserSocketId(struct User *head, char *username, int socketId);
struct User *findUserBySocketId(int socketId);
char *getUserChat(struct User *user, const char *receiverName);
char *getOnlineUserList(int currentClientSocketId);
void updateChatFile(const char *username, const char *receiver, const char *message);
void updateChatStatus(struct User *user, struct User *receiver_user, const char *message);
#endif // CHAT_H