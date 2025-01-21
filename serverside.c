#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <pthread.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024


void initialize_server();
void *handle_client(void *arg);
void send_to_client(SOCKET client_socket, const char *message);
void broadcast_message(const char *message, SOCKET sender_socket);
void receive_from_client(SOCKET client_socket, char *buffer);

SOCKET server_socket;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 

typedef struct {
    SOCKET socket;
    struct sockaddr_in addr;
    char name[50]; 
} client_info;

client_info *clients[100];  
int client_count = 0;

int main() {
 
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

  
    initialize_server();


    while (1) {
        client_info *cli_info = (client_info *)malloc(sizeof(client_info));
        if (!cli_info) {
            printf("Memory allocation error.\n");
            continue;
        }


        int client_size = sizeof(cli_info->addr);
        cli_info->socket = accept(server_socket, (struct sockaddr *)&cli_info->addr, &client_size);
        if (cli_info->socket == INVALID_SOCKET) {
            printf("Accept failed. Error Code: %d\n", WSAGetLastError());
            free(cli_info);
            continue;
        }

     
        char name[50];
        printf("Enter your name: ");
        recv(cli_info->socket, name, sizeof(name), 0);
        strcpy(cli_info->name, name);

       
        pthread_mutex_lock(&mutex);
        clients[client_count++] = cli_info;
        pthread_mutex_unlock(&mutex);

        printf("Client %s connected from %s:%d\n", name, inet_ntoa(cli_info->addr.sin_addr), ntohs(cli_info->addr.sin_port));

     
        char welcome_message[BUFFER_SIZE];
        sprintf(welcome_message, "%s has joined the chat!", name);
        broadcast_message(welcome_message, cli_info->socket);

     
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)cli_info) != 0) {
            printf("Failed to create thread for client.\n");
            free(cli_info);
        } else {
            pthread_detach(thread_id); 
        }
    }

   
    closesocket(server_socket);
    WSACleanup();

    return 0;
}

void initialize_server() {
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }


    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

  
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed. Error Code: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        exit(1);
    }

 
    if (listen(server_socket, 5) == SOCKET_ERROR) {
        printf("Listen failed. Error Code: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        exit(1);
    }

    printf("Server is listening on port %d\n", SERVER_PORT);
}

void *handle_client(void *arg) {
    client_info *cli_info = (client_info *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
 
        receive_from_client(cli_info->socket, buffer);

        if (strcmp(buffer, "exit") == 0) {
            printf("Client %s disconnected.\n", cli_info->name);
            break;
        }

     
        char message[BUFFER_SIZE];
        sprintf(message, "%s: %s", cli_info->name, buffer);
        broadcast_message(message, cli_info->socket);
    }


    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i] == cli_info) {
            clients[i] = clients[client_count - 1]; 
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

   
    closesocket(cli_info->socket);
    free(cli_info);
    return NULL;
}

void send_to_client(SOCKET client_socket, const char *message) {
    if (send(client_socket, message, strlen(message), 0) == SOCKET_ERROR) {
        printf("Send failed. Error Code: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        exit(1);
    }
}

void broadcast_message(const char *message, SOCKET sender_socket) {
    pthread_mutex_lock(&mutex);

    for (int i = 0; i < client_count; i++) {
        if (clients[i]->socket != sender_socket) { 
            send_to_client(clients[i]->socket, message);
        }
    }

    pthread_mutex_unlock(&mutex);
}

void receive_from_client(SOCKET client_socket, char *buffer) {
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received == SOCKET_ERROR) {
        printf("Receive failed. Error Code: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        exit(1);
    }
    buffer[bytes_received] = '\0'; 
}
