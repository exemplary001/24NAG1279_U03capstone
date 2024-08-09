#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdarg.h>
#include "common.h"

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_message(const char *format, ...) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (!log_file) return;

    va_list args;
    va_start(args, format);
    fprintf(log_file, "%s ", get_timestamp());
    vfprintf(log_file, format, args);
    fprintf(log_file, "\n");
    va_end(args);
    fclose(log_file);
}

char* get_timestamp() {
    static char buffer[20];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    int bytes_received;

    // Authenticate user
    send_message(client->socket, "Enter your username: ");
    recv(client->socket, client->username, sizeof(client->username), 0);
    client->username[strcspn(client->username, "\n")] = 0; // Remove newline

    // Log user login
    log_message("User %s logged in", client->username);

    snprintf(message, sizeof(message), "Welcome to the chat server, %s!\n", client->username);
    send_message(client->socket, message);

    while ((bytes_received = recv(client->socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        if (strncmp(buffer, "/private", 8) == 0) {
            char target_user[USERNAME_SIZE];
            char private_message[BUFFER_SIZE];
            sscanf(buffer, "/private %s %[^\n]", target_user, private_message);

            int found = 0;
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] && strcmp(clients[i]->username, target_user) == 0) {
                    snprintf(message, sizeof(message), "[Private from %s]: %s\n", client->username, private_message);
                    send_message(clients[i]->socket, message);
                    found = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);

            if (!found) {
                send_message(client->socket, "User not found.\n");
            }
        } else {
            snprintf(message, sizeof(message), "%s [%s]: %s\n", client->username, get_timestamp(), buffer);
            broadcast_message(message, client->socket);
        }

        // Log message
        log_message("Message from %s: %s", client->username, buffer);
    }

    // Log user logout
    log_message("User %s logged out", client->username);

    close(client->socket);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client) {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    free(client);
    return NULL;
}

void broadcast_message(const char *message, int exclude_fd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->socket != exclude_fd) {
            send_message(clients[i]->socket, message);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_message(int fd, const char *message) {
    send(fd, message, strlen(message), 0);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addr_len = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addr_len)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        client_t *new_client = malloc(sizeof(client_t));
        new_client->socket = new_socket;

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == NULL) {
                clients[i] = new_client;
                pthread_create(&new_client->thread, NULL, handle_client, (void *)new_client);
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    return 0;
}
