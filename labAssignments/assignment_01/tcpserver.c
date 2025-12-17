#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    
    listen(server_fd, 3);
    
    printf("Server is listening on port %d...\n", PORT);
    
    int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    read(new_socket, buffer, BUFFER_SIZE);
    printf("Client says: %s\n", buffer);
    
    char *response = "Hello";
    if (strcmp(buffer, "Hi") == 0) {
        send(new_socket, response, strlen(response), 0);
        printf("Server sent: %s\n", response);
    }

    close(new_socket);
    close(server_fd);

    return 0;
}
