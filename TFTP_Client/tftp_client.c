#include "tftp.h"
#include "tftp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Function declarations
bool validate_ip_address(char *ip);
bool validate_file_present(char *file_name);

char mode[10];
int data_size;

int main()
{
    char command[256];                  // Buffer to store user command
    tftp_client_t client;               // Structure to hold client details
    memset(&client, 0, sizeof(client)); // Initialize all values to 0

    strcpy(mode, "default");
    data_size = 512;

    printf("Type help to get supported features\n");

    // Infinite loop for command line interface
    while (1)
    {
        printf("tftp> ");
        fgets(command, sizeof(command), stdin); // Read command from user

        // Remove newline character added by fgets
        command[strcspn(command, "\n")] = 0;

        // Send command to processor function
        process_command(&client, command);
    }

    return 0;
}

// Function to process user commands
void process_command(tftp_client_t *client, char *command)
{
    char *cmd = strtok(command, " "); // Extract first word (command)

    if (cmd == NULL)
        return;

    // If user types help
    if (strcmp("help", cmd) == 0)
    {
        print_help();
    }

    // If user types quit or bye
    else if (strcmp("bye", cmd) == 0 || strcmp("quit", cmd) == 0)
    {
        disconnect(client);
        exit(0); // Exit program
    }

    // If user types connect <ip>
    else if (strcmp("connect", cmd) == 0)
    {
        char *ip = strtok(NULL, " "); // Get IP address

        bool valid = validate_ip_address(ip); // Validate IP format

        if (valid)
        {
            connect_to_server(client, ip, PORT);
            printf("INFO: Connected to server\n");
        }
        else
        {
            printf("Usage: connect <ip>\n");
        }
    }

    // If user types put <filename>
    else if (strcmp("put", cmd) == 0)
    {
        char *file_name = strtok(NULL, " "); // Get filename

        bool valid_file = validate_file_present(file_name); // Check file exists

        if (valid_file)
        {
            put_file(client, file_name); // Start file upload
        }
        else
        {
            printf("ERROR: File is not present\n");
        }
    }

    // If user types get <filename>
    else if (strcmp("get", cmd) == 0)
    {
        char *file_name = strtok(NULL, " "); // Get filename

        get_file(client, file_name); // Start file download
    }

    else if (strcmp("mode", cmd) == 0)
    {
        char *mode_name = strtok(NULL, " ");

        if (mode_name == NULL)
        {
            printf("Current mode is %s\n", mode);
            return;
        }

        if (strcmp(mode_name, "default") == 0)
        {
            strcpy(mode, "default");
            data_size = 512;
        }
        else if (strcmp(mode_name, "octet") == 0)
        {
            strcpy(mode, "octet");
            data_size = 1;
        }
        else if (strcmp(mode_name, "netascii") == 0)
        {
            strcpy(mode, "netascii");
            data_size = 512;
        }
        else
        {
            printf("Unsupported mode\n");
            return;
        }

        printf("Mode changed to %s | Block size = %d\n", mode, data_size);
    }
}

// This function only creates socket and stores server details
// It does NOT send any packet to server
void connect_to_server(tftp_client_t *client, char *ip, int port)
{
    // Create UDP socket
    client->sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Fill server address details
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(PORT);          // Convert port to network byte order
    client->server_addr.sin_addr.s_addr = inet_addr(ip); // Convert IP string to binary / network byte order

    strcpy(client->server_ip, ip); // Store IP in structure
    client->server_ip[strlen(ip)] = '\0';
    client->server_len = sizeof(client->server_addr); // Store size of address structure
}

// Function to send file to server
void put_file(tftp_client_t *client, char *filename)
{
    // Check if socket is created
    if (client->sockfd <= 0)
    {
        printf("Not connected to server\n");
        return;
    }

    // Send WRQ (Write Request) to server
    send_request(client->sockfd, client->server_addr, filename, WRQ);
}

// Function to receive file from server
void get_file(tftp_client_t *client, char *filename)
{
    // Send RRQ (Read Request) to server
    send_request(client->sockfd, client->server_addr, filename, RRQ);
}

void disconnect(tftp_client_t *client)
{
    // Close socket
    close(client->sockfd);
}

// Function to send RRQ or WRQ packet to server
void send_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode)
{
    tftp_packet packet;
    memset(&packet, 0, sizeof(packet)); // Clear packet memory

    packet.opcode = htons(opcode);                  // Convert opcode to network byte order
    strcpy(packet.body.request.filename, filename); // Copy filename
    strcpy(packet.body.request.mode, mode);
    printf("File name is : %s\n", filename);

    if (opcode == WRQ)
        printf("Sending WRQ to server...\n");
    else
        printf("Sending RRQ to server...\n");

    // Send packet to server using UDP
    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Wait for server response
    receive_request(sockfd, server_addr, filename, opcode);
}

// Function to handle server response
void receive_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode)
{
    tftp_packet response;
    socklen_t len = sizeof(server_addr);

    // If Write Request
    if (opcode == WRQ)
    {
        // Wait for ACK from server
        int n = recvfrom(sockfd, &response, sizeof(response), 0, (struct sockaddr *)&server_addr, &len);

        if (n < 0)
        {
            perror("recvfrom failed");
            return;
        }

        // If ACK with block number 0, server is ready
        if (ntohs(response.opcode) == ACK && ntohs(response.body.ack_packet.block_number) == 0)
        {
            printf("Server ready. Starting file upload...\n");
            send_file(sockfd, server_addr, len, filename);
        }
        // If server sends error
        else if (ntohs(response.opcode) == ERROR)
        {
            printf("Server ERROR: %s\n", response.body.error_packet.error_msg);
            return;
        }
        else
        {
            printf("Invalid ACK received\n");
        }
    }

    // If Read Request
    else if (opcode == RRQ)
    {
        printf("Receiving file...\n");
        receive_file(sockfd, server_addr, len, filename);
        printf("File download completed\n");
    }
}

// Prints available commands
void print_help()
{
    printf("connect <server-ip>    : Connect to server\n");
    printf("get <file-name>        : Receive a file from server\n");
    printf("put <file-name>        : Send a file to the server\n");
    printf("mode                   : Transfer mode(s) (octet, netascii or default)\n");
    printf("bye / quit             : Exit the application\n");
}

// Validate IP format (returns true if valid IPv4)
// bool validate_ip_address(char *ip)
// {
//     if (ip == NULL)
//         return false;

//     struct sockaddr_in sa;

//     // Convert IP string to binary and check validity
//     int result = inet_pton(AF_INET, ip, &(sa.sin_addr));

//     return (result == 1);
// }

bool validate_ip_address(char *ip)
{
    if (ip == NULL)
        return false;

    int num = 0;  // To store each octet value
    int dots = 0; // Count dots
    int len = strlen(ip);

    if (len < 7 || len > 15) // Minimum: 0.0.0.0 (7 chars)
        return false;

    for (int i = 0; i < len; i++)
    {
        if (ip[i] >= '0' && ip[i] <= '9')
        {
            num = num * 10 + (ip[i] - '0');

            if (num > 255) // Each octet must be <= 255
                return false;
        }
        else if (ip[i] == '.')
        {
            dots++;

            if (dots > 3) // Only 3 dots allowed
                return false;

            num = 0; // Reset for next octet
        }
        else
        {
            return false; // Invalid character
        }
    }

    return (dots == 3); // Valid only if exactly 3 dots
}

// Check whether file exists or not
bool validate_file_present(char *file_name)
{
    int fd = open(file_name, O_RDONLY);

    if (fd == -1)
        return false;

    close(fd); // Close file if opened
    return true;
}
