#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Helper function to process the operation
double calculate(char *op, double val1, double val2) {
    if (strcmp(op, "add") == 0) return val1 + val2;
    if (strcmp(op, "sub") == 0) return val1 - val2;
    if (strcmp(op, "mul") == 0) return val1 * val2;
    if (strcmp(op, "div") == 0) return (val2 != 0) ? val1 / val2 : NAN;
    if (strcmp(op, "sin") == 0) return sin(val1);
    if (strcmp(op, "cos") == 0) return cos(val1);
    if (strcmp(op, "tan") == 0) return tan(val1);
    if (strcmp(op, "log") == 0) return log(val1);
    if (strcmp(op, "sqrt") == 0) return sqrt(val1);
    if (strcmp(op, "inv") == 0) return (val1 != 0) ? 1.0 / val1 : NAN;
    return NAN;
}

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Scientific Calculator Server is running on port %d...\n", PORT);

    while (1) {
        socklen_t len = sizeof(cliaddr);
        int n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';

        printf("Client requested: %s\n", buffer);

        char op[20];
        double val1 = 0, val2 = 0, result = 0;
        int args = sscanf(buffer, "%s %lf %lf", op, &val1, &val2);

        result = calculate(op, val1, val2);

        char response[BUFFER_SIZE];
        if (isnan(result)) {
            sprintf(response, "Error: Invalid Operation or Math Error");
        } else {
            sprintf(response, "Result: %.4f", result);
        }

        sendto(sockfd, (const char *)response, strlen(response), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
        printf("Sent response: %s\n", response);
    }

    return 0;
}
