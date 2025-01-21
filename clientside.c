#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <pthread.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024


void initialize_connection();
void send_to_server(const char *message);
void receive_from_server(char *buffer);
void *receive_messages(void *arg);
void *send_messages(void *arg);

SOCKET client_socket;

int main() {

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

   
    initialize_connection();

    
    pthread_t receive_thread, send_thread;
    pthread_create(&receive_thread, NULL, receive_messages, NULL);
    pthread_create(&send_thread, NULL, send_messages, NULL);

   
    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);

    
    closesocket(client_socket);
    WSACleanup();

    return 0;
}

void initialize_connection() {
    struct sockaddr_in server_addr;


    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

  
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connection failed. Error Code: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        exit(1);
    }

    printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

 
    char name[50];
    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0'; 
    send_to_server(name); 
}

void send_to_server(const char *message) {
    if (send(client_socket, message, strlen(message), 0) == SOCKET_ERROR) {
        printf("Send failed. Error Code: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        exit(1);
    }
}

void receive_from_server(char *buffer) {
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received == SOCKET_ERROR) {
        printf("Receive failed. Error Code: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        exit(1);
    }
    buffer[bytes_received] = '\0';  
}

void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        receive_from_server(buffer);
        printf("%s\n", buffer);  
    }
}

void *send_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  
        send_to_server(buffer); 
    }
}
