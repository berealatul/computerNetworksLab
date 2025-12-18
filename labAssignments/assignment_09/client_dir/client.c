#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

/**
 * Function to receive file from server (Download)
 */
void receive_file(int socket, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("[-]Error in creating file.");
        return;
    }

    char buffer[BUFFER_SIZE];
    int n;
    clock_t start, end;
    double cpu_time_used;

    printf("[+]Downloading file from server...\n");
    start = clock();

    while (1) {
        n = recv(socket, buffer, BUFFER_SIZE, 0);
        if (n <= 0 || buffer[0] == '\0') {
            break;
        }
        fprintf(fp, "%s", buffer);
        memset(buffer, 0, BUFFER_SIZE);
    }

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("[+]Download complete. Saved as %s. Time taken: %f seconds\n", filename, cpu_time_used);

    fclose(fp);
}

/**
 * Function to send file to server (Upload)
 */
void send_file(int socket, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("[-]Error in reading file.");
        return;
    }

    char data[BUFFER_SIZE] = {0};
    clock_t start, end;
    double cpu_time_used;

    printf("[+]Uploading file: %s\n", filename);
    start = clock();

    while (fgets(data, BUFFER_SIZE, fp) != NULL) {
        if (send(socket, data, sizeof(data), 0) == -1) {
            perror("[-]Error in sending file.");
            break;
        }
        memset(data, 0, BUFFER_SIZE);
    }

    // Send end-of-file marker
    memset(data, 0, BUFFER_SIZE);
    send(socket, data, sizeof(data), 0);

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("[+]Upload complete. Time taken: %f seconds\n", cpu_time_used);

    fclose(fp);
}

int main(int argc, char const *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    const char* server_ip = "10.0.0.1"; // Default to localhost

    if (argc > 1) {
        server_ip = argv[1];
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // 1. Download file from server
    receive_file(sock, "downloaded_from_server.txt");

    // 2. Upload file to server
    send_file(sock, "client_file.txt");

    close(sock);

    return 0;
}