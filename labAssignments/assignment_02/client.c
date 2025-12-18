#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    char buffer[BUFFER_SIZE] = {0};
    char fruit_name[50];
    int quantity;
    char message[BUFFER_SIZE];

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, "10.0.0.1", &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // clear buffer and read inventory from server
    memset(buffer, 0, BUFFER_SIZE);
    if (read(sock, buffer, BUFFER_SIZE) > 0) {
        printf("%s", buffer);
    }

    // user shopping
    printf("Enter fruit name to buy: ");
    scanf("%s", fruit_name);
    printf("Enter quantity: ");
    scanf("%d", &quantity);

    snprintf(message, sizeof(message), "%s %d", fruit_name, quantity);
    send(sock, message, strlen(message), 0);
    printf("\n[Request Sent]: Buying %d %s(s)...\n", quantity, fruit_name);

    // transaction result
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_read = read(sock, buffer, BUFFER_SIZE);
    if (bytes_read > 0) {
        printf("[Server Response]: %s\n", buffer);
    } else {
        printf("No response from server.\n");
    }

    close(sock);
    return 0;
}
