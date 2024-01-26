#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <limits.h>
#include <arpa/inet.h>
#include <pthread.h>

void startListeningAndPrintMessagesOnNewThread(int fd);

void *listenAndPrint(void *p_socketFD);
int check(int exp, const char *msg);

void readConsoleEntriesAndSendToServer(int socketFD);
typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;
#define SERVERPORT 8989
#define SOCKETERROR (-1)
#define MAX_SIZE 50

bool login_accpeted = 0;

int main()
{

    int server_socket, client_socket, addr_size;
    SA_IN server_addr, cli;

    // socket create and verification
    check((server_socket = socket(AF_INET, SOCK_STREAM, 0)), "Failed to create connection");

    printf("Socket successfully created..\n");

    bzero(&server_addr, sizeof(server_addr));

    // assign IP, PORT
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVERPORT);

    // connect the client socket to server socket
    check(connect(server_socket, (SA *)&server_addr, sizeof(server_addr)), "connection with the server failed...\n");
    printf("connected to the server..\n");

    startListeningAndPrintMessagesOnNewThread(server_socket);

    readConsoleEntriesAndSendToServer(server_socket);

    close(server_socket);

    return 0;
}

void readConsoleEntriesAndSendToServer(int socketFD)
{
    char auth_buffer[1024];
    char username[MAX_SIZE] = "User1";
    char password[MAX_SIZE] = "password1";
    int socket_id = 5;

    // printf("Enter username: ");
    // scanf("%s", username);
    // printf("Enter password: ");
    // scanf("%s", password);

    char *line = NULL;
    size_t lineSize = 0;

    printf("type and we will send(type exit)...\n");
    char buffer[1024];

    while (true)
    {
        // send  login  request to server
        if (login_accpeted == 0)
        {
            sprintf(auth_buffer, "login|%s|%s", username, password);
            printf("auth message sent\n");
            ssize_t amountWasSent = send(socketFD,
                                         auth_buffer,
                                         strlen(auth_buffer), 0);
            // chnage it to
            login_accpeted = 1;
        }
        else
        {
            ssize_t charCount = getline(&line, &lineSize, stdin);
            line[charCount - 1] = 0;

            if (charCount > 0)
            {

                if (strcmp(line, "exit") == 0)
                    break;

                else
                {
                    sprintf(buffer, "send|%d|%s", socket_id, line);
                    printf("message sent");
                    ssize_t amountWasSent = send(socketFD,
                                                 buffer,
                                                 strlen(buffer), 0);
                }
            }
        }
    }
}

void startListeningAndPrintMessagesOnNewThread(int socketFD)
{

    pthread_t id;
    int *pclient = malloc(sizeof(int));
    *pclient = socketFD;
    pthread_create(&id, NULL, listenAndPrint, pclient);
}

void *listenAndPrint(void *p_socketFD)
{
    int socketFD = *((int *)p_socketFD);

    char buffer[1024];

    while (true)
    {
        ssize_t amountReceived = recv(socketFD, buffer, 1024, 0);

        login_accpeted = 1;

        if (amountReceived > 0)
        {
            buffer[amountReceived] = 0;
            printf("Response was %s\n ", buffer);
        }

        if (amountReceived == 0)
            break;
    }

    close(socketFD);
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