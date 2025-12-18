#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 2048

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];

// Utility to clear current line and redraw prompt
void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}

// Utility to remove newline character
void str_trim_lf(char* arr, int length) {
    int i;
    for (i = 0; i < length; i++) {
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

// Signal handler for clean exit
void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}

void send_msg_handler() {
    char message[BUFFER_SIZE] = {};

    while (1) {
        str_overwrite_stdout();
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) break;
        str_trim_lf(message, BUFFER_SIZE);

        if (strcmp(message, "exit") == 0) {
            break;
        } else if (strlen(message) > 0) {
            // Append newline for server processing if needed, 
            // though the server logic adds timestamps and newlines.
            send(sockfd, message, strlen(message), 0);
        }

        bzero(message, BUFFER_SIZE);
    }
    catch_ctrl_c_and_exit(2);
}

void recv_msg_handler() {
    char message[BUFFER_SIZE] = {};
    while (1) {
        int receive = recv(sockfd, message, BUFFER_SIZE, 0);
        if (receive > 0) {
            // Clear current prompt line before printing incoming message
            printf("\r%*s\r", 40, ""); // Crude way to clear prompt line
            printf("%s", message);
            // If the message doesn't end in a newline, add one for clarity
            if (message[strlen(message) - 1] != '\n') {
                printf("\n");
            }
            str_overwrite_stdout();
        } else if (receive == 0) {
            break;
        } else {
            // Error case
            break;
        }
        memset(message, 0, sizeof(message));
    }
}

int main(int argc, char **argv) {
    char *ip = "10.0.0.1";
    int port = 8080;

    // Handle signals
    signal(SIGINT, catch_ctrl_c_and_exit);

    printf("Please enter your name: ");
    fgets(name, 32, stdin);
    str_trim_lf(name, 32);

    if (strlen(name) > 31 || strlen(name) < 2) {
        printf("Name must be between 2 and 31 characters.\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;

    // Socket settings
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // Connect to Server
    int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err == -1) {
        printf("ERROR: connect failed\n");
        return EXIT_FAILURE;
    }

    // Send Name
    send(sockfd, name, 32, 0);

    printf("=== WELCOME TO THE CHATROOM ===\n");

    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) != 0) {
        printf("ERROR: pthread creation failed\n");
        return EXIT_FAILURE;
    }

    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) != 0) {
        printf("ERROR: pthread creation failed\n");
        return EXIT_FAILURE;
    }

    while (1) {
        if (flag) {
            printf("\nExiting chatroom...\n");
            break;
        }
    }

    close(sockfd);

    return EXIT_SUCCESS;
}
