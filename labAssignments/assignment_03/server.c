#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CUSTOMERS 100

typedef struct {
    char name[20];
    int quantity;
    char last_sold[50];
} Fruit;

typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
} Customer;

Fruit inventory[] = {
    {"Apple", 20, "Never"},
    {"Banana", 20, "Never"},
    {"Mango", 20, "Never"}
};

int fruit_count = 3;

Customer customers[MAX_CUSTOMERS];
int unique_customer_count = 0;

void get_timestamp(char *buffer) {
    time_t t;
    struct tm *tmp;
    time(&t);
    tmp = localtime(&t);
    strftime(buffer, 50, "%Y-%m-%d %H:%M:%S", tmp);
}

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    
    struct sockaddr_in servaddr, cliaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    
    // bind
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));

    printf("UDP Fruit Server started on port %d...\n", PORT);

    while (1) {
        socklen_t len = sizeof(cliaddr);
        memset(buffer, 0, BUFFER_SIZE);
        memset(response, 0, BUFFER_SIZE);

        // message from client
        int n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, 0, 
                        (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';

        // client ip and port
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cliaddr.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(cliaddr.sin_port);

        // Case A: Client asks for Inventory (Handshake)
        if (strcmp(buffer, "MENU") == 0) {
            char inventory_msg[BUFFER_SIZE] = "\n--- Current Inventory ---\n";
            for (int i = 0; i < fruit_count; i++) {
                char line[64];
                snprintf(line, sizeof(line), "%s: %d\n", inventory[i].name, inventory[i].quantity);
                strcat(inventory_msg, line);
            }
            strcat(inventory_msg, "-------------------------\n");
            
            // send current inventory to client
            sendto(sockfd, (const char *)inventory_msg, strlen(inventory_msg), 0, 
                  (const struct sockaddr *)&cliaddr, len);
        } 
        
        // Case B: Client wants to buy (Transaction)
        else {
            char req_fruit[20];
            int req_qty;

            // Parse "FruitName Quantity"
            if (sscanf(buffer, "%s %d", req_fruit, &req_qty) != 2) {
                snprintf(response, BUFFER_SIZE, "Invalid format. Use: Name Quantity");
            } else {
                int fruit_index = -1;
                for (int i = 0; i < fruit_count; i++) {
                    if (strcasecmp(inventory[i].name, req_fruit) == 0) {
                        fruit_index = i;
                        break;
                    }
                }

                if (fruit_index != -1 && inventory[fruit_index].quantity >= req_qty && req_qty != 0) {
                    // update inventory
                    inventory[fruit_index].quantity -= req_qty;
                    get_timestamp(inventory[fruit_index].last_sold);

                    // unique customer
                    int is_new = 1;
                    int customer_index = -1;

                    for (int i = 0; i < unique_customer_count; i++) {
                        if (strcmp(customers[i].ip, client_ip) == 0) {
                            is_new = 0;
                            customer_index = i;
                            break;
                        }
                    }

                    if (is_new && unique_customer_count < MAX_CUSTOMERS) {
                        strcpy(customers[unique_customer_count].ip, client_ip);
                        customers[unique_customer_count].port = client_port;
                        unique_customer_count++;
                    } else if (!is_new && customer_index != -1) {
                         // update customer port for existing IP
                        customers[customer_index].port = client_port;
                    }

                    snprintf(response, BUFFER_SIZE, "SUCCESS: Sold %d %s(s). Total Unique Customers: %d", 
                            req_qty, inventory[fruit_index].name, unique_customer_count);
                    
                    // Log on Server
                    printf("[Transaction] Sold %d %s to %s:%d\n", 
                           req_qty, inventory[fruit_index].name, client_ip, client_port);
                    
                    // Display Unique Customers
                    printf("--- Unique Customers History (by IP) ---\n");
                    for(int i=0; i<unique_customer_count; i++) {
                        printf("%d. %s (Latest Port: %d)\n", i+1, customers[i].ip, customers[i].port);
                    }
                    printf("----------------------------------------\n");

                } else {
                    if (fruit_index == -1) {
                        snprintf(response, BUFFER_SIZE, "REGRET: %s not found.", req_fruit);
                    } else {
                        snprintf(response, BUFFER_SIZE, "REGRET: Only %d %s(s) available.", 
                                inventory[fruit_index].quantity, inventory[fruit_index].name);
                    }
                }
            }

            // Send response back
            sendto(sockfd, (const char *)response, strlen(response), 0, 
                  (const struct sockaddr *)&cliaddr, len);
        }
    }
    
    return 0;
}
