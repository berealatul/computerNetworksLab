#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "10.0.0.1"
#define PORT 5001
#define DATA_SIZE (2 * 1024 * 1024) // 2MB
#define NUM_THREADS 8
#define CHUNK_SIZE 4096

// dummy data
void fill_buffer(char *buf, int size, int thread_id) {
    for (int i = 0; i < size; i++) {
        buf[i] = (char)((i + thread_id) % 256);
    }
}

void *client_thread(void *arg) {
    int thread_id = *(int *)arg;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    char *data_buffer;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // convert IPv4 and IPv6 addresses from text to binary form
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    printf("[Thread %d] Connected. Preparing to send 2MB...\n", thread_id);

    // allocate 2MB buffer
    data_buffer = (char *)malloc(DATA_SIZE);
    fill_buffer(data_buffer, DATA_SIZE, thread_id);

    // send data
    int total_sent = 0;
    int sent;
    while (total_sent < DATA_SIZE) {
        int to_send = (DATA_SIZE - total_sent) > CHUNK_SIZE ? CHUNK_SIZE : (DATA_SIZE - total_sent);
        sent = send(sock, data_buffer + total_sent, to_send, 0);
        total_sent += sent;
    }

    printf("[Thread %d] Finished sending %d bytes.\n", thread_id, total_sent);

    free(data_buffer);
    close(sock);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    printf("Starting %d concurrent client threads...\n", NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, client_thread, &thread_ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All threads finished.\n");
    return 0;
}
