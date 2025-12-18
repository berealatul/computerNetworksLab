#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

void process_packet(unsigned char* buffer, int size);
void print_ethernet_header(unsigned char* buffer);
void print_ip_header(unsigned char* buffer);
void print_tcp_header(unsigned char* buffer, int ip_header_len);
void print_payload(unsigned char* buffer, int size);

int tcp_count = 0;
int total_count = 0;

int main() {
    int sock_raw;
    int saddr_size, data_size;
    struct sockaddr saddr;
    unsigned char *buffer = (unsigned char *)malloc(65536); // Max MTU size

    printf("Starting Network Sniffer...\n");

    /* * Step 1: Create a raw socket to listen for all packets at the Ethernet level.
     * ETH_P_ALL tells the kernel to give us every packet the interface sees.
     */
    sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_raw < 0) {
        perror("Socket Error (Are you running as root?)");
        return 1;
    }

    printf("Listening for packets... Press Ctrl+C to stop.\n\n");

    while (1) {
        saddr_size = sizeof(saddr);
        /* Receive a packet */
        data_size = recvfrom(sock_raw, buffer, 65536, 0, &saddr, (socklen_t*)&saddr_size);
        if (data_size < 0) {
            perror("Recvfrom error");
            break;
        }

        total_count++;
        process_packet(buffer, data_size);
    }

    close(sock_raw);
    free(buffer);
    return 0;
}

// Dissects the raw packet buffer to extract protocol layers.
void process_packet(unsigned char* buffer, int size) {
    // Extract IP Header (Starts after Ethernet header which is 14 bytes)
    struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));

    // We are specifically interested in TCP (Protocol 6)
    if (iph->protocol == 6) {
        tcp_count++;
        printf("--- [ Packet #%d | TCP Packet #%d ] ---\n", total_count, tcp_count);
        
        print_ethernet_header(buffer);
        print_ip_header(buffer);
        
        int ip_header_len = iph->ihl * 4;
        print_tcp_header(buffer, ip_header_len);        
        printf("\n");
    }
}

// Prints Ethernet Layer information.
void print_ethernet_header(unsigned char* buffer) {
    struct ethhdr *eth = (struct ethhdr *)buffer;
    printf("Ethernet Header\n");
    printf(" |-Source Address      : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5]);
    printf(" |-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n", eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
    printf(" |-Protocol            : %u\n", (unsigned short)eth->h_proto);
}

// Prints IP Layer information
void print_ip_header(unsigned char* buffer) {
    struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));
    struct sockaddr_in source, dest;
    
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;

    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;

    printf("IP Header\n");
    printf(" |-IP Version        : %d\n", (unsigned int)iph->version);
    printf(" |-IP Header Length  : %d DWORDS or %d Bytes\n", (unsigned int)iph->ihl, ((unsigned int)(iph->ihl)) * 4);
    printf(" |-Type Of Service   : %d\n", (unsigned int)iph->tos);
    printf(" |-Total Length      : %d Bytes\n", ntohs(iph->tot_len));
    printf(" |-TTL               : %d\n", (unsigned int)iph->ttl);
    printf(" |-Protocol          : %d\n", (unsigned int)iph->protocol);
    printf(" |-Checksum          : %d\n", ntohs(iph->check));
    printf(" |-Source IP         : %s\n", inet_ntoa(source.sin_addr));
    printf(" |-Destination IP    : %s\n", inet_ntoa(dest.sin_addr));
}

// Prints TCP Layer information.
void print_tcp_header(unsigned char* buffer, int ip_header_len) {
    // TCP Header starts after Ethernet (14 bytes) and IP Header
    struct tcphdr *tcph = (struct tcphdr *)(buffer + ip_header_len + sizeof(struct ethhdr));

    printf("TCP Header\n");
    printf(" |-Source Port          : %u\n", ntohs(tcph->source));
    printf(" |-Destination Port     : %u\n", ntohs(tcph->dest));
    printf(" |-Sequence Number      : %u\n", ntohl(tcph->seq));
    printf(" |-Acknowledge Number   : %u\n", ntohl(tcph->ack_seq));
    printf(" |-Header Length        : %d DWORDS or %d Bytes\n", (unsigned int)tcph->doff, (unsigned int)tcph->doff * 4);
    printf(" |-Window Size          : %u\n", ntohs(tcph->window));
    printf(" |-Checksum             : %u\n", ntohs(tcph->check));
    printf(" |-Urgent Pointer       : %u\n", tcph->urg_ptr);
    
    printf(" |-Flags                : ");
    if(tcph->urg) printf("URG ");
    if(tcph->ack) printf("ACK ");
    if(tcph->psh) printf("PSH ");
    if(tcph->rst) printf("RST ");
    if(tcph->syn) printf("SYN ");
    if(tcph->fin) printf("FIN ");
    printf("\n");
}

// Prints the data payload in hex and ASCII.
void print_payload(unsigned char* buffer, int size) {
    printf("Payload Data\n");
    for (int i = 0; i < size; i++) {
        if (i != 0 && i % 16 == 0) printf("\n");
        printf(" %.2X", buffer[i]);
    }
    printf("\n");
}
