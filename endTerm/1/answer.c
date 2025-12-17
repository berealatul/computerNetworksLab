#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <pwd.h>

// configuration
#define SRC_MAC {0x00, 0x00, 0x00, 0x00, 0x00, 0x01}
#define DST_MAC {0x00, 0x00, 0x00, 0x00, 0x00, 0x02}
#define SRC_IP  "10.0.0.1"
#define DST_IP  "10.0.0.2"
#define INTERFACE "h1-eth0"
#define DST_PORT 12345
#define SRC_PORT 54321
#define ROLL_NUMBER "CSM24006"
#define PACKET_LEN 1024

int create_raw_socket(const char *iface_name, int *if_index) {
    int sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    
    struct ifreq if_idx;
    memset(&if_idx, 0, sizeof(struct ifreq));
    strncpy(if_idx.ifr_name, iface_name, IFNAMSIZ-1);
    ioctl(sockfd, SIOCGIFINDEX, &if_idx);

    *if_index = if_idx.ifr_ifindex;
    return sockfd;
}

// c) The payload must contain your Roll Number and the current system username and time.
int generate_payload(char *buffer) {
    struct passwd *pw = getpwuid(getuid());
    time_t now = time(NULL);

    return sprintf(buffer,
            "Roll: %s | User: %s | Time: %s",
            ROLL_NUMBER,
            pw->pw_name,
            ctime(&now));
}

// a) Manually construct the Ethernet Header, IP Header, and UDP Header.
// b) You must deliberately calculate an incorrect UDP checksum and inject it into the header.
void build_headers(char *buffer, int payload_len) {
    struct ether_header *eh = (struct ether_header *) buffer;
    struct iphdr *iph = (struct iphdr *) (buffer + sizeof(struct ether_header));
    struct udphdr *udph = (struct udphdr *) (buffer + sizeof(struct ether_header) + sizeof(struct iphdr));

    // udp header
    udph->source = htons(SRC_PORT);
    udph->dest = htons(DST_PORT);
    udph->len = htons(sizeof(struct udphdr) + payload_len);
    
    // deliberately calculate an incorrect UDP checksum
    udph->check = htons(0xBAD1);

    // ip header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + payload_len);
    iph->id = htonl(54321);
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_UDP;
    iph->saddr = inet_addr(SRC_IP);
    iph->daddr = inet_addr(DST_IP);
    iph->check = 0; 
    iph->check = 0x1234;

    // ethernet header
    uint8_t src_mac[6] = SRC_MAC;
    uint8_t dst_mac[6] = DST_MAC;
    memcpy(eh->ether_shost, src_mac, 6);
    memcpy(eh->ether_dhost, dst_mac, 6);
    eh->ether_type = htons(ETHERTYPE_IP);
}

void send_packet(int sockfd, 
    int if_index, 
    char *buffer, 
    int total_len) 
{
    struct sockaddr_ll socket_address;
    uint8_t dst_mac[6] = DST_MAC;

    socket_address.sll_family = AF_PACKET;
    socket_address.sll_ifindex = if_index;
    socket_address.sll_halen = ETH_ALEN;
    memcpy(socket_address.sll_addr, dst_mac, 6);

    sendto(sockfd, 
        buffer, 
        total_len, 
        0, 
        (struct sockaddr*)&socket_address, 
        sizeof(struct sockaddr_ll));
}

int main() {
    int sockfd, if_index;
    char buffer[PACKET_LEN];
    memset(buffer, 0, PACKET_LEN);

    sockfd = create_raw_socket(INTERFACE, &if_index);
    
    int header_size = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct udphdr);
    int payload_len = generate_payload(buffer + header_size);
    
    build_headers(buffer, payload_len);
    send_packet(sockfd, if_index, buffer, header_size + payload_len);
    close(sockfd);
}
