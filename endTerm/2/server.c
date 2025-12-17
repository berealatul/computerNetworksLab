#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>

#define PORT 5001
#define BUFFER_SIZE 4096

typedef struct {
    int socket_fd;
    int client_id;
} client_handler_args;

int client_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    client_handler_args *args = (client_handler_args *)arg;
    int sock = args->socket_fd;
    int id = args->client_id;
    free(args);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    struct timeval tv;
    long long total_bytes = 0;

    while ((bytes_read = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        gettimeofday(&tv, NULL);
        // timestamp in micrroseconds
        unsigned long long timestamp = (unsigned long long)(tv.tv_sec) * 1000000 + tv.tv_usec;
        
        // we need to buffer logs to avoid I/O blocking
        printf("[%llu] Client %d: Received %zd bytes\n", timestamp, id, bytes_read);
        
        total_bytes += bytes_read;
    }

    if (bytes_read == 0) {
        printf("[Server] Client %d disconnected. Total received: %lld bytes\n", id, total_bytes);
    } else if (bytes_read < 0) {
        perror("recv failed");
    }

    close(sock);
    return NULL;
}

int main() {
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // attach socket to the port 5001
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);

    printf("[Server] Listening on port %d...\n", PORT);

    int new_socket;
    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

        pthread_mutex_lock(&count_mutex);
        int current_id = ++client_count;
        pthread_mutex_unlock(&count_mutex);

        // allocate memory for thread args
        client_handler_args *args = malloc(sizeof(client_handler_args));
        args->socket_fd = new_socket;
        args->client_id = current_id;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, (void *)args);
        pthread_detach(thread_id);
    }

    return 0;
}
