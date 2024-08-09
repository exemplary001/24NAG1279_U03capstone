#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "common.h"

void *receive_messages(void *sockfd) {
    int socket = *(int *)sockfd;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }

    printf("Server disconnected.\n");
    exit(0);
}

int main() {
    int sockfd;
    struct sockaddr_in server_address;
    char username[USERNAME_SIZE];
    char message[BUFFER_SIZE];
    pthread_t recv_thread;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Authenticate the user
    printf("Enter your username: ");
    fgets(username, USERNAME_SIZE, stdin);
    username[strcspn(username, "\n")] = 0; // Remove newline
    send(sockfd, username, strlen(username), 0);

    pthread_create(&recv_thread, NULL, receive_messages, (void *)&sockfd);

    while (1) {
        fgets(message, sizeof(message), stdin);

        if (strncmp(message, "/private", 8) == 0) {
            send(sockfd, message, strlen(message), 0);
        } else {
            send(sockfd, message, strlen(message), 0);
        }
    }

    close(sockfd);
    return 0;
}
