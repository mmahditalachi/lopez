#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <limits.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "chat.h"

#define SERVERPORT 8989
#define BUFSIZE 4098
#define SOCKETERROR (-1)
#define SERVER_BACKLOG 1
#define MAX_SIZE 50
#define MAX_CHAT_SIZE 1000

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

void *handle_connection(void *p_client_socket);
int checkCredentials(char *username, char *password);
int login(char *username, char *password);
void displayList();
int startup();
char **split_string(char *str, char *delim);
// ERROR HANDLIG FUNCTION -1: INDICATE ERROR
int check(int exp, const char *msg);

struct User *userList = NULL;

int main()
{
    startup();

    int server_socket, client_socket, addr_size;
    SA_IN server_addr, client_addr;

    // SOCK_STREAM : HANDLING STREAM DATA INSTEAD SEVERAL PACKET OF DATA
    check((server_socket = socket(AF_INET, SOCK_STREAM, 0)), "Failed to create connection");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVERPORT);

    check(bind(server_socket, (SA *)&server_addr, sizeof(server_addr)), "Bind Failed");
    check(listen(server_socket, SERVER_BACKLOG), "Listen Failed");

    while (true)
    {
        printf("waiting for connection..\n");

        addr_size = sizeof(SA_IN);
        check(client_socket = accept(server_socket, (SA *)&client_addr, (socklen_t *)&addr_size), "accept failed");

        printf("Connected: %d \n", client_socket);

        pthread_t t;
        int *pclient = malloc(sizeof(int));
        *pclient = client_socket;
        pthread_create(&t, NULL, handle_connection, pclient);
    }
}

int check(int exp, const char *msg)
{
    if (exp == SOCKETERROR)
    {
        perror(msg);
        exit(1);
    }
    return exp;
}

void *handle_connection(void *p_client_socket)
{
    int client_socket = *((int *)p_client_socket);
    free(p_client_socket);
    char read_buffer[MAX_CHAT_SIZE];
    memset(read_buffer, '\0', sizeof(read_buffer));

    size_t byte_read;
    int msgsize = 0;

    // read each line to get new line char
    while (true)
    {
        memset(read_buffer, '\0', sizeof(read_buffer));
        byte_read = recv(client_socket, read_buffer, sizeof(read_buffer), 0);
        if (byte_read > 0)
        {
            printf("SocketId %d\n", client_socket);
            printf("Request %s\n", read_buffer);

            char delim[] = "|";
            char **tokens = split_string(read_buffer, delim);

            struct User *user = findUserBySocketId(client_socket);
            if (user == NULL)
            {
                int result = login(tokens[1], tokens[2]);
                if (result == 1)
                {
                    char result[1024];
                    setUserSocketId((struct User *)userList, tokens[1], client_socket);
                    char *onlineUsers = getOnlineUserList(client_socket);
                    sprintf(result, "ok|login|user login accepted \n write user id to start chat \n online users:\n %s", onlineUsers);
                    send(client_socket, result, strlen(result), 0);
                }
                else
                {
                    char result[] = "error|login|user login faild";
                    send(client_socket, result, strlen(result), 0);
                }
            }
            else
            {
                int receiver_socket = atoi(tokens[1]);
                struct User *receiver_user = findUserBySocketId(receiver_socket);
                if (receiver_user == NULL)
                {
                    char result[] = "error|send|user is offline";
                    send(client_socket, result, strlen(result), 0);
                }
                else if (receiver_user->sockID == client_socket)
                {
                    char result[] = "error|send|Invalid user, You can not select your current user";
                    send(client_socket, result, strlen(result), 0);
                }
                else if (strcmp(tokens[0], "newsend") == 0 || strcmp(tokens[0], "send") == 0)
                {

                    char result[MAX_CHAT_SIZE];
                    char receiver_result[MAX_CHAT_SIZE];

                    if (strcmp(tokens[2], "\n") != 0)
                        updateChatStatus(user, receiver_user, tokens[2]);

                    char *new_chat = getUserChat(user, receiver_user->username);

                    sprintf(receiver_result, "ok|send|%s\nEnter Message:|%d", new_chat, user->sockID);
                    sprintf(result, "ok|send|%s|%d", new_chat, receiver_user->sockID);

                    send(receiver_socket, receiver_result, strlen(receiver_result), 0);
                    send(client_socket, result, strlen(result), 0);

                    free(new_chat);
                }
            }

            free(tokens);

            if (byte_read == 0)
            {
                printf("end");
                break;
            }
        }
    }
    close(client_socket);

    printf("closing connection");
    close(client_socket);
    return NULL;
}

int checkCredentials(char *username, char *password)
{
    FILE *file;
    char fileUsername[MAX_SIZE], filePassword[MAX_SIZE];

    file = fopen("user_credentials.txt", "r");

    if (file == NULL)
    {
        printf("Error opening file.\n");
        return 0;
    }
    int retry = 0;

    while (fscanf(file, "%s   %s", fileUsername, filePassword) == 2)
    {
        if (strcmp(username, fileUsername) == 0 && strcmp(password, filePassword) == 0)
        {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

int login(char *username, char *password)
{
    if (checkCredentials(username, password))
    {
        printf("Login successful. %s \n", username);
        return 1;
    }

    printf("Invalid username or password.\n");
    return 0;
}

void addMessage(struct Message **history, const char *username, const char *receiver, const char *message)
{
    struct Message *newMessage = (struct Message *)malloc(sizeof(struct Message));
    if (newMessage == NULL)
    {
        perror("Error allocating memory for message");
        exit(EXIT_FAILURE);
    }

    strcpy(newMessage->sender, username);
    strcpy(newMessage->receiver, receiver);
    strcpy(newMessage->message, message);
    newMessage->next = NULL;

    if (*history == NULL)
    {
        *history = newMessage;
    }
    else
    {
        struct Message *current = *history;
        while (current->next != NULL)
        {
            current = current->next;
        }

        current->next = newMessage;
    }
}

void addUser(struct User **head, const char *username, const char *datetime)
{
    struct User *newNode = (struct User *)malloc(sizeof(struct User));
    strcpy(newNode->username, username);
    strcpy(newNode->datetime, datetime);
    newNode->sockID = 0;
    newNode->chatHistory = NULL; // Initialize chat history as an empty list
    newNode->next = *head;
    *head = newNode;
}

void displayList()
{
    printf("displayList\n");
    struct User *current = (struct User *)userList;
    while (current != NULL)
    {
        printf("Username: %s\n", current->username);
        printf("Datetime: %s\n", current->datetime);

        // Display chat history for the current user
        struct Message *message = current->chatHistory;
        while (message != NULL)
        {
            printf("   %s: %s\n", message->sender, message->message);
            message = message->next;
        }

        printf("\n");

        current = current->next;
    }
}

int startup()
{
    // Read user information from user.tsv file
    FILE *userFile = fopen("user.tsv", "r");
    if (userFile == NULL)
    {
        perror("Error opening user.tsv");
        return 1;
    }

    char sendername[50];
    char receiver[50];
    char datetime[50];

    while (fscanf(userFile, "%s    %s", sendername, datetime) == 2)
    {
        printf("user: %s \n", sendername);
        addUser(&userList, sendername, datetime);
    }

    fclose(userFile);

    // Read chat information from chat.tsv file
    FILE *chatFile = fopen("chat.tsv", "r");
    if (chatFile == NULL)
    {
        perror("Error opening chat.tsv");
        return 1;
    }

    char message[100];

    while (fscanf(chatFile, "%49s\t%49s\t%150[^\n]%*c", sendername, receiver, message) == 3)
    {
        struct User *currentUser = (struct User *)userList;
        while (currentUser != NULL &&
               (strcmp(currentUser->username, sendername) != 0 ||
                strcmp(currentUser->username, receiver) != 0))
        {
            if (currentUser != NULL)
            {
                if (strcmp(currentUser->username, sendername) == 0 || strcmp(currentUser->username, receiver) == 0)
                    addMessage(&(currentUser->chatHistory), sendername, receiver, message);
            }
            else
            {
                printf("Warning: Message for unknown user %s\n", sendername);
            }
            currentUser = currentUser->next;
        }
    }

    fclose(chatFile);

    // seed users and messages test
    // Displaying the linked list
    // displayList();

    return 0;
}

struct User *findUserByName(char username[])
{
    struct User *currentUser = (struct User *)userList;
    while (currentUser->next != NULL)
    {
        if (strcmp(currentUser->username, username) == 0)
            break;

        currentUser = currentUser->next;
    }

    if (strcmp(currentUser->username, username) != 0)
        return NULL;

    return currentUser;
}

void setUserSocketId(struct User *head, char *username, int socketId)
{
    while (head != NULL)
    {
        if (strcmp(head->username, username) == 0)
        {
            head->sockID = socketId;
            return;
        }
        head = head->next;
    }

    printf("Warning: setUserSocketId User %s not found.\n", username);
}

struct User *findUserBySocketId(int socketId)
{
    struct User *currentUser = (struct User *)userList;
    while (currentUser != NULL)
    {
        if (currentUser->sockID == socketId)
            return currentUser;

        currentUser = currentUser->next;
    }

    return NULL;
}

char *getUserChat(struct User *user, const char *receiverName)
{
    char *chat = malloc(MAX_CHAT_SIZE * sizeof(char));
    struct User *currentUser = (struct User *)user;
    struct Message *message = currentUser->chatHistory;
    while (message != NULL)
    {
        if ((strcmp(message->receiver, currentUser->username) == 0 && strcmp(message->sender, receiverName) == 0) ||
            (strcmp(message->receiver, receiverName) == 0 && strcmp(message->sender, currentUser->username) == 0))
        {
            strcat(chat, message->sender);
            strcat(chat, ":");
            strcat(chat, message->message);
            strcat(chat, "\n");
        }

        message = message->next;
    }
    return chat;
}

char **split_string(char *str, char *delim)
{
    char **tokens = (char **)malloc(MAX_CHAT_SIZE * sizeof(char *));
    int n = 0;

    char *token = strtok(str, delim);

    // delimiters present in str[].
    while (token != NULL)
    {
        printf(" %s\n", token);
        tokens[n] = token;
        token = strtok(NULL, delim);
        n++;
    }

    return tokens;
}
char *getOnlineUserList(int currentClientSocketId)
{
    char *onlineUser = malloc(MAX_SIZE * sizeof(char));
    struct User *currentUser = (struct User *)userList;
    memset(onlineUser, '\0', sizeof(onlineUser));
    while (currentUser != NULL)
    {
        char sss[1000];
        int sock = currentUser->sockID;
        char *username = currentUser->username;
        if (currentUser->sockID == currentClientSocketId)
            sprintf(sss, "%d\t%s\t(current user)\n",
                    sock, username);
        else if (currentUser->sockID == 0)
            sprintf(sss, "%d\t%s\t(offline)\n",
                    sock, username);
        else
            sprintf(sss, "%d\t%s\n",
                    sock, username);
        strcat(onlineUser, sss);
        currentUser = currentUser->next;
    }

    return onlineUser;
}

void updateChatFile(const char *username, const char *receiver, const char *message)
{
    FILE *file = fopen("chat.tsv", "a");

    if (file == NULL)
    {
        perror("Error opening file");
        return;
    }

    fprintf(file, "\n%s\t%s\t%s", username, receiver, message);
    fclose(file);
}

void updateChatStatus(struct User *user, struct User *receiver_user, const char *message)
{
    updateChatFile(user->username, receiver_user->username, message);
    addMessage(&(user->chatHistory), user->username, receiver_user->username, message);
    addMessage(&(receiver_user->chatHistory), user->username, receiver_user->username, message);
}