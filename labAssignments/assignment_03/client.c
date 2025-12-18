#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);

    inet_pton(AF_INET, "10.0.0.1", &servaddr.sin_addr);
    socklen_t len = sizeof(servaddr);

    // Send "MENU" to initiate contact and get inventory
    // In UDP, there is no connect(), so we must send data to tell server we exist
    char *menu_req = "MENU";
    sendto(sockfd, (const char *)menu_req, strlen(menu_req), 0, 
           (const struct sockaddr *)&servaddr, sizeof(servaddr));

    // receive inventory
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, 0, 
                     (struct sockaddr *)&servaddr, &len);
    buffer[n] = '\0';
    printf("%s", buffer);

    // user shoping
    char fruit_name[50];
    int quantity;
    printf("Enter fruit name to buy: ");
    scanf("%s", fruit_name);
    printf("Enter quantity: ");
    scanf("%d", &quantity);

    // transaction message
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "%s %d", fruit_name, quantity);

    // send request
    sendto(sockfd, (const char *)message, strlen(message), 0, 
           (const struct sockaddr *)&servaddr, sizeof(servaddr));
    printf("\n[Request Sent]: Buying %d %s(s)...\n", quantity, fruit_name);

    // get response
    memset(buffer, 0, BUFFER_SIZE);
    n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, 0, 
                 (struct sockaddr *)&servaddr, &len);
    buffer[n] = '\0';
    printf("[Server Response]: %s\n", buffer);

    close(sockfd);
    return 0;
}
