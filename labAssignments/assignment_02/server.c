#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
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

pthread_mutex_t lock;

void get_timestamp(char *buffer) {
    time_t t;
    struct tm *tmp;
    time(&t);
    tmp = localtime(&t);
    strftime(buffer, 50, "%Y-%m-%d %H:%M:%S", tmp);
}

void *handle_client(void *socket_desc) {
    int sock = *(int*)socket_desc;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getpeername(sock, (struct sockaddr*)&addr, &addr_len);
    
    char buffer[BUFFER_SIZE] = {0};
    char response[BUFFER_SIZE] = {0};
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(addr.sin_port);

    free(socket_desc);

    // prepare inventory data
    char inventory_msg[BUFFER_SIZE] = "\n--- Current Inventory ---\n";
    pthread_mutex_lock(&lock);
    for (int i = 0; i < fruit_count; i++) {
        char line[64];
        snprintf(line, sizeof(line), "%s: %d\n", inventory[i].name, inventory[i].quantity);
        strcat(inventory_msg, line);
    }
    pthread_mutex_unlock(&lock);
    strcat(inventory_msg, "-------------------------\n");
    
    // send inventory to client
    send(sock, inventory_msg, strlen(inventory_msg), 0);

    // read request
    int valread = read(sock, buffer, BUFFER_SIZE);
    if (valread <= 0) {
        close(sock);
        return NULL;
    }

    char req_fruit[20];
    int req_qty;
    
    // fruit name parser
    if (sscanf(buffer, "%s %d", req_fruit, &req_qty) != 2) {
        snprintf(response, BUFFER_SIZE, "Invalid format. Use: Name Quantity");
        send(sock, response, strlen(response), 0);
        close(sock);
        return NULL;
    }

    // critical section start here
    pthread_mutex_lock(&lock);

    int fruit_index = -1;
    // find fruit
    for (int i = 0; i < fruit_count; i++) {
        if (strcasecmp(inventory[i].name, req_fruit) == 0) {
            fruit_index = i;
            break;
        }
    }

    // process transaction
    if (fruit_index != -1 && inventory[fruit_index].quantity >= req_qty) {
        
        // update inventory
        inventory[fruit_index].quantity -= req_qty;
        get_timestamp(inventory[fruit_index].last_sold);
        
        int is_new = 1;
        int customer_index = -1;
        for (int i = 0; i < unique_customer_count; i++) {
            // Unique by IP
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
            // existing customer port update
            customers[customer_index].port = client_port;
        }

        // success message
        snprintf(response, BUFFER_SIZE, "SUCCESS: Sold %d %s(s). Total Unique Customers: %d", 
                 req_qty, inventory[fruit_index].name, unique_customer_count);
        
        // print server transaction
        printf("[Transaction] Sold %d %s to %s:%d at %s\n", 
               req_qty, inventory[fruit_index].name, client_ip, client_port, inventory[fruit_index].last_sold);
        
        // display current customer list
        printf("--- Unique Customers History (by IP) ---\n");
        for(int i=0; i<unique_customer_count; i++) {
            printf("%d. %s (Latest Port: %d)\n", i+1, customers[i].ip, customers[i].port);
        }
        printf("----------------------------------------\n");

    } else {
        // Regret Message
        if (fruit_index == -1) {
            snprintf(response, BUFFER_SIZE, "REGRET: %s not found in inventory.", req_fruit);
        } else {
            snprintf(response, BUFFER_SIZE, "REGRET: Only %d %s(s) available.", 
                     inventory[fruit_index].quantity, inventory[fruit_index].name);
        }
    }

    pthread_mutex_unlock(&lock);
    // critical section ends here

    send(sock, response, strlen(response), 0);
    close(sock);
    return NULL;
}

int main() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    pthread_mutex_init(&lock, NULL);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0)
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Fruit Server started on port %d...\n", PORT);
    printf("Initial Inventory:\n");
    for(int i=0; i<fruit_count; i++) 
        printf("- %s: %d\n", inventory[i].name, inventory[i].quantity);

    int new_socket;
    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

        // create new thread
        pthread_t thread_id;
        int *pclient = malloc(sizeof(int));
        *pclient = new_socket;
        
        pthread_create(&thread_id, NULL, handle_client, pclient);
        pthread_detach(thread_id);
    }

    pthread_mutex_destroy(&lock);
    close(server_fd);
    return 0;
}
