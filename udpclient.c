#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 4096
#define SERVER_PORT 2018

typedef struct {
    int packet_id;
    int packet_size;
    char data[BUF_SIZE];
} frame_packet;

void handle_error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <protocol_type (1:Stop-and-Wait, 2:GBN)>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int protocol_type = atoi(argv[2]);

    int sockfd;
    struct sockaddr_in servaddr;
    socklen_t len;
    frame_packet frame;
    int ack;
    FILE *fp;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        handle_error("Socket creation failed");
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    servaddr.sin_addr.s_addr = inet_addr(server_ip);

    // Send connection request (just an int as a placeholder)
    ack = 0;
    sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    printf("Connected to server.\n");

    fp = fopen("received_file", "wb");
    if (fp == NULL) {
        handle_error("File open failed");
    }

    while (1) {
        len = sizeof(servaddr);
        recvfrom(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *)&servaddr, &len);

        // Check for end of transmission
        if (strcmp(frame.data, "EOF") == 0) {
            printf("File transfer complete.\n");
            break;
        }

        // Write the data to file
        fwrite(frame.data, 1, frame.packet_size, fp);

        // Send acknowledgment (Stop-and-Wait Protocol)
        if (protocol_type == 1) {
            ack = frame.packet_id;
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&servaddr, len);
        }

        printf("Packet %d received, %d bytes written.\n", frame.packet_id, frame.packet_size);
    }

    fclose(fp);
    close(sockfd);
    return 0;
}
