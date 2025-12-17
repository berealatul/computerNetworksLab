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
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, "10.0.0.1", &serv_addr.sin_addr); // mininet server ip for host 1
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    
    char *message = "Hi";
    send(sock, message, strlen(message), 0);
    printf("Client sent: %s\n", message);

    read(sock, buffer, BUFFER_SIZE);
    printf("Server says: %s\n", buffer);

    close(sock);
    return 0;
}
