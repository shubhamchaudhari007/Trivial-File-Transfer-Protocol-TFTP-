/* Common file for both server & client */

#include "tftp.h"
#include <fcntl.h>

// These variables are defined in client/server file
// We are just accessing them here
extern char mode[10];
extern int data_size;

/* ===========================================================
   Function: send_file()
   Purpose : Sends file data block-by-block to receiver
   =========================================================== */
void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename)
{
    tftp_packet packet, ack_packet;

    // Open file in read-only mode
    int fd = open(filename, O_RDONLY);

    if (fd < 0)
    {
        perror("File open failed");
        return;
    }

    int block_number = 1; // TFTP block numbers start from 1
    int bytes_read;

    // Keep sending data until file is completely sent
    while (1)
    {
        memset(&packet, 0, sizeof(packet)); // Clear previous packet data

        if (!strcmp(mode, "netascii"))
        {
            int i = 0;
            char ch;

            while (i < data_size)
            {
                if (read(fd, &ch, 1) <= 0)
                    break;

                if (ch == '\n')
                {
                    packet.body.data_packet.data[i++] = '\r';
                    if (i < data_size)
                        packet.body.data_packet.data[i++] = '\n';
                }
                else
                {
                    packet.body.data_packet.data[i++] = ch;
                }
            }
            bytes_read = i;
        }
        else
        {
            // Read data_size bytes from file (dynamic block size)
            bytes_read = read(fd, packet.body.data_packet.data, data_size);
        }

        if (bytes_read < 0)
        {
            perror("Read error");
            close(fd);
            return;
        }

        // Set packet type to DATA
        packet.opcode = htons(DATA);

        // Set block number (convert to network byte order)
        packet.body.data_packet.block_number = htons(block_number);

        // Send DATA packet
        // Header = 4 bytes (opcode + block number)
        sendto(sockfd, &packet, 4 + bytes_read, 0, (struct sockaddr *)&client_addr, client_len);

        printf("Sent DATA block %d (%d bytes)\n", block_number, bytes_read);

        // Wait for ACK from receiver
        memset(&ack_packet, 0, sizeof(ack_packet));

        recvfrom(sockfd, &ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)&client_addr, &client_len);

        // If ACK is wrong, resend the same block
        while (ntohs(ack_packet.opcode) != ACK || ntohs(ack_packet.body.ack_packet.block_number) != block_number)
        {
            printf("Resending block %d\n", block_number);

            sendto(sockfd, &packet, 4 + bytes_read, 0, (struct sockaddr *)&client_addr, client_len);

            recvfrom(sockfd, &ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)&client_addr, &client_len);
        }

        // If bytes_read is less than block size,
        // this means it is the last packet
        if (bytes_read < data_size)
            break;

        block_number++; // Move to next block
    }

    printf("File transfer completed successfully\n");

    close(fd); // Close file after transfer
}

/* ===========================================================
   Function: receive_file()
   Purpose : Receives file data block-by-block from sender
   =========================================================== */
void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename)
{
    tftp_packet packet, ack_packet;

    // Open file for writing
    // Create file if not present
    // Truncate if already exists
    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);

    if (fd < 0)
    {
        perror("File open failed");
        return;
    }

    int expected_block = 1; // First DATA block should be 1
    int n;

    // Keep receiving packets until file transfer completes
    while (1)
    {
        memset(&packet, 0, sizeof(packet)); // Clear old packet data

        // Receive DATA packet
        n = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &client_len);

        if (n < 0)
        {
            perror("recvfrom failed");
            close(fd);
            return;
        }

        // Check if received packet is DATA type
        if (ntohs(packet.opcode) != DATA)
        {
            printf("Invalid packet received\n");
            close(fd);
            return;
        }

        int block_number = ntohs(packet.body.data_packet.block_number);

        // Process only if block number matches expected block
        if (block_number == expected_block)
        {
            // Actual data size = total received bytes - 4 header bytes
            int received_size = n - 4;

            if(strcmp(mode, "netascii") == 0)
            {
                char buffer[data_size];
                int j = 0;

                for(int i = 0; i < received_size; i++)
                {
                    if(i + 1 < received_size && packet.body.data_packet.data[i] == '\r' && packet.body.data_packet.data[i + 1] == '\n')
                    {
                        buffer[j++] = '\n';
                        i++;
                    }
                    else
                    {
                        buffer[j++] = packet.body.data_packet.data[i];
                    }
                }
                write(fd, buffer, j);
            }
            else
            {
                // Write received data to file
                write(fd, packet.body.data_packet.data, received_size);
            }
            printf("Received DATA block %d (%d bytes)\n", block_number, received_size);

            // Prepare ACK packet
            memset(&ack_packet, 0, sizeof(ack_packet));

            ack_packet.opcode = htons(ACK);
            ack_packet.body.ack_packet.block_number = htons(block_number);

            // Send ACK back to sender
            sendto(sockfd, &ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)&client_addr, client_len);

            expected_block++; // Expect next block

            // If received data is less than block size,
            // this means this was the last packet
            if (received_size < data_size)
            {
                break;
            }
        }
    }

    printf("File received successfully\n");

    close(fd); // Close file after transfer
}
