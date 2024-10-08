#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

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

// Simulate packet loss
int should_drop_packet(float loss_probability) {
    float random_value = ((float)rand()) / RAND_MAX;
    return random_value < loss_probability;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <file_name> <loss_probability> <protocol_type (1:Stop-and-Wait, 2:GBN)>\n", argv[0]);
        return 1;
    }

    char *file_name = argv[1];
    float loss_probability = atof(argv[2]);
    int protocol_type = atoi(argv[3]);

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    frame_packet frame;
    int ack;

    FILE *fp = fopen(file_name, "rb");
    if (fp == NULL) {
        handle_error("File open failed");
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        handle_error("Socket creation failed");
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        handle_error("Bind failed");
    }

    printf("Server is waiting for client...\n");
    len = sizeof(cliaddr);
    recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&cliaddr, &len); // waiting for connection
    printf("Client connected.\n");

    int packet_id = 0, total_size = 0, bytes_read;
    while ((bytes_read = fread(frame.data, 1, BUF_SIZE, fp)) > 0) {
        frame.packet_id = packet_id++;
        frame.packet_size = bytes_read;

        // Simulate packet loss
        if (should_drop_packet(loss_probability)) {
            printf("Packet %d dropped.\n", frame.packet_id);
            continue;
        }

        // Send the packet to the client
        sendto(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *)&cliaddr, len);

        // Wait for acknowledgment (Stop-and-Wait Protocol)
        if (protocol_type == 1) {
            recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&cliaddr, &len);
            if (ack != frame.packet_id) {
                printf("Packet %d not acknowledged, resending...\n", frame.packet_id);
                fseek(fp, -bytes_read, SEEK_CUR); // Go back to resend
            } else {
                printf("Packet %d acknowledged.\n", frame.packet_id);
            }
        }
        total_size += bytes_read;
    }

    // Notify end of transmission
    strcpy(frame.data, "EOF");
    frame.packet_size = strlen("EOF");
    sendto(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *)&cliaddr, len);
    printf("File transmission complete. Total bytes sent: %d\n", total_size);

    fclose(fp);
    close(sockfd);
    return 0;
}
