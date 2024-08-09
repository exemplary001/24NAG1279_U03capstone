#ifndef COMMON_H
#define COMMON_H

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define USERNAME_SIZE 50
#define LOG_FILE "server.log"

// Structure to hold client information
typedef struct {
    int socket;
    char username[USERNAME_SIZE];
    pthread_t thread;
} client_t;

// Function declarations
void log_message(const char *format, ...);
void send_message(int fd, const char *message);
void broadcast_message(const char *message, int exclude_fd);
char* get_timestamp();

#endif
