#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048

// Structure to hold client information
typedef struct {
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[32];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to get current timestamp string
void get_timestamp(char *buffer) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 32, "[%Y-%m-%d %H:%M:%S]", t);
}

// Write message to log.txt
void write_to_log(const char *message) {
    pthread_mutex_lock(&log_mutex);
    FILE *fp = fopen("log.txt", "a");
    if (fp != NULL) {
        fprintf(fp, "%s\n", message);
        fclose(fp);
    }
    pthread_mutex_unlock(&log_mutex);
}

// Add clients to the queue
void queue_add(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Remove clients from the queue
void queue_remove(int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->uid == uid) {
                clients[i] = NULL;
                break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Send message to all clients except the sender
void send_message(char *s, int uid) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->uid != uid) {
                if (write(clients[i]->sockfd, s, strlen(s)) < 0) {
                    perror("ERROR: write to descriptor failed");
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Handle all communication with the client
void *client_handler(void *arg) {
    char buff_out[BUFFER_SIZE];
    char name[32];
    int leave_flag = 0;

    client_t *cli = (client_t *)arg;

    // Receive Name
    if (recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 || strlen(name) >= 32 - 1) {
        printf("Didn't enter the name.\n");
        leave_flag = 1;
    } else {
        strcpy(cli->name, name);
        char timestamp[32];
        get_timestamp(timestamp);
        sprintf(buff_out, "%s %s has joined\n", timestamp, cli->name);
        printf("%s", buff_out);
        send_message(buff_out, cli->uid);
        write_to_log(buff_out);
    }

    bzero(buff_out, BUFFER_SIZE);

    while (1) {
        if (leave_flag) {
            break;
        }

        int receive = recv(cli->sockfd, buff_out, BUFFER_SIZE, 0);
        if (receive > 0) {
            if (strlen(buff_out) > 0) {
                char final_msg[BUFFER_SIZE + 64];
                char timestamp[32];
                get_timestamp(timestamp);
                
                sprintf(final_msg, "%s %s: %s", timestamp, cli->name, buff_out);
                send_message(final_msg, cli->uid);
                write_to_log(final_msg);
                
                // Print to server console
                printf("%s", final_msg);
            }
        } else if (receive == 0 || strcmp(buff_out, "exit") == 0) {
            char timestamp[32];
            get_timestamp(timestamp);
            sprintf(buff_out, "%s %s has left\n", timestamp, cli->name);
            printf("%s", buff_out);
            send_message(buff_out, cli->uid);
            write_to_log(buff_out);
            leave_flag = 1;
        } else {
            printf("ERROR: -1\n");
            leave_flag = 1;
        }

        bzero(buff_out, BUFFER_SIZE);
    }

    // Delete client from queue and yield thread
    close(cli->sockfd);
    queue_remove(cli->uid);
    free(cli);
    pthread_detach(pthread_self());

    return NULL;
}

int main(int argc, char **argv) {
    int port = 8080;
    int sockfd, newfd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    pthread_t thread_id;
    static int uid = 10;

    // Socket settings
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR: Socket binding failed");
        return EXIT_FAILURE;
    }

    // Listen
    if (listen(sockfd, 10) < 0) {
        perror("ERROR: Socket listening failed");
        return EXIT_FAILURE;
    }

    printf("=== WELCOME TO THE CHATROOM ===\n");
    printf("Server started on port %d\n", port);

    while (1) {
        socklen_t clilen = sizeof(client_addr);
        newfd = accept(sockfd, (struct sockaddr *)&client_addr, &clilen);

        // Check if max clients is reached
        if ((uid - 10) >= MAX_CLIENTS) {
            printf("Max clients reached. Rejected: ");
            printf("%s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            close(newfd);
            continue;
        }

        // Client settings
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = client_addr;
        cli->sockfd = newfd;
        cli->uid = uid++;

        // Add client to the queue and fork thread
        queue_add(cli);
        if (pthread_create(&thread_id, NULL, &client_handler, (void *)cli) != 0) {
            perror("ERROR: pthread_create failed");
        }

        // Reduce CPU usage
        sleep(1);
    }

    return EXIT_SUCCESS;
}
