#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 4096  // Maximum packet size
#define SERVER_PORT 8080  // Server's port

typedef struct {
    int packet_id;        // ID to track the packet sequence
    int packet_size;      // Size of the data in this packet
    char data[BUF_SIZE];  // Data buffer for the file content
} frame_packet;

// Function to handle errors and terminate the program
void handle_error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    // Ensure correct usage: three command-line arguments (server_hostname, file_name, protocol_type)
    if (argc != 4) {
        printf("Usage: %s <server_hostname> <file_name> <protocol_type (1:Stop-and-Wait, 2:GBN)>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];   // Server's IP address
    char *file_name = argv[2];   // Name of the file to request from the server
    int protocol_type = atoi(argv[3]);  // Convert protocol type to integer

    int sockfd;
    struct sockaddr_in servaddr;  // Server address structure
    socklen_t len;
    frame_packet frame;  // Frame structure to receive data
    int ack;  // Variable to store acknowledgment
    FILE *fp;  // File pointer to save the received file

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        handle_error("Socket creation failed");
    }

    // Initialize server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    servaddr.sin_addr.s_addr = inet_addr(server_ip);  // Convert server IP to network format

    // Send the requested file name to the server
    sendto(sockfd, file_name, strlen(file_name) + 1, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    printf("Requested file: %s\n", file_name);

    // Open the file to save the received data
    fp = fopen("received_file", "wb");
    if (fp == NULL) {
        handle_error("File open failed");
    }

    while (1) {
        len = sizeof(servaddr);

        // Receive a packet from the server
        recvfrom(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *)&servaddr, &len);

        // Check if the received packet is the "EOF" marker, which indicates end of transmission
        if (strcmp(frame.data, "EOF") == 0) {
            printf("File transfer complete.\n");
            break;
        }

        // Write the received data to the file
        fwrite(frame.data, 1, frame.packet_size, fp);

        // Send acknowledgment for Stop-and-Wait protocol
        if (protocol_type == 1) {
            ack = frame.packet_id;
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&servaddr, len);
        }

        printf("Packet %d received, %d bytes written.\n", frame.packet_id, frame.packet_size);
    }

    fclose(fp);  // Close the file
    close(sockfd);  // Close the socket
    return 0;
}
