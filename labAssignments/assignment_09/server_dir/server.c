#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

/**
 * Function to send a file to the client (Download from server)
 */
void send_file(int client_socket, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("[-]Error in reading file.");
        return;
    }

    char data[BUFFER_SIZE] = {0};
    clock_t start, end;
    double cpu_time_used;

    printf("[+]Sending file: %s\n", filename);
    start = clock();

    while (fgets(data, BUFFER_SIZE, fp) != NULL) {
        if (send(client_socket, data, sizeof(data), 0) == -1) {
            perror("[-]Error in sending file.");
            break;
        }
        memset(data, 0, BUFFER_SIZE);
    }
    
    // Send end-of-file marker (empty buffer)
    memset(data, 0, BUFFER_SIZE);
    send(client_socket, data, sizeof(data), 0);

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("[+]File sent successfully. Time taken: %f seconds\n", cpu_time_used);

    fclose(fp);
}

/**
 * Function to receive a file from the client (Upload to server)
 */
void receive_file(int client_socket, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("[-]Error in creating file.");
        return;
    }

    char buffer[BUFFER_SIZE];
    int n;
    clock_t start, end;
    double cpu_time_used;

    printf("[+]Receiving file from client...\n");
    start = clock();

    while (1) {
        n = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (n <= 0 || buffer[0] == '\0') {
            break;
        }
        fprintf(fp, "%s", buffer);
        memset(buffer, 0, BUFFER_SIZE);
    }

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("[+]File received successfully as %s. Time taken: %f seconds\n", filename, cpu_time_used);

    fclose(fp);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("[+]Server listening on port %d\n", PORT);

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // 1. Send file to client (Client Download)
    send_file(new_socket, "server_file.txt");

    // 2. Receive file from client (Client Upload)
    receive_file(new_socket, "uploaded_by_client.txt");

    close(new_socket);
    close(server_fd);

    return 0;
}