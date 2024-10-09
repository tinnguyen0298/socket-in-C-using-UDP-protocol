#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

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

// Function to simulate packet loss based on a given loss probability
int should_drop_packet(float loss_probability) {
    float random_value = ((float)rand()) / RAND_MAX;  // Generate a random number between 0 and 1
    return random_value < loss_probability;  // Return 1 (true) if the packet should be dropped
}

int main(int argc, char *argv[]) {
    // Ensure correct usage: two command-line arguments (loss_probability, protocol_type)
    if (argc != 3) {
        printf("Usage: %s <loss_probability> <protocol_type (1:Stop-and-Wait, 2:GBN)>\n", argv[0]);
        return 1;
    }

    float loss_probability = atof(argv[1]);  // Convert loss probability to float
    int protocol_type = atoi(argv[2]);       // Convert protocol type to integer

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;  // Server and client address structures
    socklen_t len;
    frame_packet frame;  // Frame structure to send data
    int ack;  // Variable to store acknowledgment
    char requested_file[BUF_SIZE];  // To store the requested file name from the client

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        handle_error("Socket creation failed");
    }

    // Initialize server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces

    // Bind socket to the server's address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        handle_error("Bind failed");
    }

    printf("Server is waiting for client...\n");
    len = sizeof(cliaddr);

    // Receive the requested file name from the client
    recvfrom(sockfd, requested_file, BUF_SIZE, 0, (struct sockaddr *)&cliaddr, &len); 
    printf("Client requested file: %s\n", requested_file);

    // Open the requested file
    FILE *fp = fopen(requested_file, "rb");
    if (fp == NULL) {
        handle_error("Requested file open failed");
    }

    int packet_id = 0, total_size = 0, bytes_read;
    // Read file and send in chunks
    while ((bytes_read = fread(frame.data, 1, BUF_SIZE, fp)) > 0) {
        frame.packet_id = packet_id++;  // Increment the packet ID
        frame.packet_size = bytes_read;  // Store the number of bytes read

        // Simulate packet loss with the specified probability
        if (should_drop_packet(loss_probability)) {
            printf("Packet %d dropped.\n", frame.packet_id);
            continue;  // Skip sending this packet
        }

        // Send the packet to the client
        sendto(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *)&cliaddr, len);

        // Handle Stop-and-Wait protocol: wait for acknowledgment from client
        if (protocol_type == 1) {
            recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&cliaddr, &len);
            if (ack != frame.packet_id) {  // If acknowledgment does not match, resend the packet
                printf("Packet %d not acknowledged, resending...\n", frame.packet_id);
                fseek(fp, -bytes_read, SEEK_CUR);  // Move the file pointer back to resend
            } else {
                printf("Packet %d acknowledged.\n", frame.packet_id);
            }
        }
        total_size += bytes_read;  // Keep track of total bytes sent
    }

    // After all packets are sent, send an "EOF" marker to indicate the end of transmission
    strcpy(frame.data, "EOF");
    frame.packet_size = strlen("EOF");
    sendto(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *)&cliaddr, len);
    printf("File transmission complete. Total bytes sent: %d\n", total_size);

    fclose(fp);  // Close the file
    close(sockfd);  // Close the socket
    return 0;
}
