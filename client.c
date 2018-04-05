#include <stdio.h>					// for print() and fprintf()
#include <sys/socket.h>				// for socket(), connect(), sendto(), and recvfrom()
#include <arpa/inet.h>				// for sockaddr_in and inet_addr()
#include <stdlib.h>					// for atoi()
#include <string.h>					// for memset()
#include <unistd.h>					// for close()

#define BUFFER_SIZE	 200			// Buffer size

void DieWithError(char* errorMessage) {
	// Error handling function
	perror(errorMessage);
	exit(1);
}

int main(int argc, char* argv[]) {
	int sock;							// Socket descriptor
	struct sockaddr_in serverAddress;	// Server address
	struct sockaddr_in fromAddress;		// Source address of the received message
	unsigned short serverPort;			// Server port
	unsigned int fromSize;				// In-out of address size for recvfrom()
	char* serverIP;						// IP address of server
	char* filePath;						// Name (or path of file to send)
	char buffer[BUFFER_SIZE];			// Buffer for receiving messages
	unsigned long bytesToSend;			// Number of bytes to send to the server
	unsigned long bytesSent;			// Number of bytes sent
	unsigned char ack;
	unsigned char positiveAck = 1;
	char serverResponse;		// Server response (Success or Format error)
	unsigned long bytesToReceive;		// Number of bytes to receive from server
	unsigned long bytesReceived;		// Number of bytes received

	if ((argc != 4)) { 					// Test for correct number of arguments
		fprintf(stderr, "Usage: %s <Server IP> <Server Port> <File Path>\n", argv[0]);
		exit(1);
	}

	serverIP = argv[1];					// First arg: server IP address (dotted quad)
	serverPort = atoi(argv[2]);			// Second arg: server port number
	filePath = argv[3];					// Third arg: file to send

	// Create a datagram/UDP socket
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	// Construct the server address structure
	memset(&serverAddress, 0, sizeof(serverAddress));			// Zero out structure
	serverAddress.sin_family = AF_INET;							// Internet addr family
	serverAddress.sin_addr.s_addr = inet_addr(serverIP);		// Server IP address
	serverAddress.sin_port = htons(serverPort);					// Server Port

	printf("Sending message to server...\n");

	// Try to open the file at file path 
	FILE* in = fopen(filePath, "rb");
	if (in == NULL) {
		perror("Failed to open file");
		return -1;
	}

	// Get the size of the file in bytes
	fseek(in, 0L, SEEK_END);
	unsigned long fileSize = ftell(in);
	printf("File Size: %lu\n", fileSize);
	rewind(in);

	// Initialize bytesToSend
	bytesToSend = sizeof(long);

	// Send the file size to the server
	do {
		// Retransmit until the file size is sent correctly
		printf("Sending file size...\n");
		bytesSent = sendto(sock, &fileSize, bytesToSend, 0, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
	} while (bytesSent != bytesToSend);
	printf("Sent fileSize to server ...\n");

	// Get fromSize
	fromSize = sizeof(fromAddress);

	// Set bytesToReceive
	bytesToReceive = sizeof(char);

	// Receive acknowledgement
	if ((bytesReceived = recvfrom(sock, &ack, bytesToReceive, 0, (struct sockaddr *) &fromAddress, &fromSize)) != bytesToReceive)
		DieWithError("recvfrom() failed for fileSize ACK");
	
	printf("Received ACK for fileSize...\n");

	// Set bytesToSend
	bytesToSend = fileSize;
	printf("Sending file to server ...\n");
	// Send the file in chunks up to the buffer size
	int count = 1;
	while (bytesToSend > 0) {
		if (bytesToSend > BUFFER_SIZE) {
			fread(buffer, 1, BUFFER_SIZE, in);
			// Retransmit until the data is sent correctly
			do {
				printf("Sending chunk %d...\n", count);
				bytesSent = sendto(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
			} while (bytesSent != BUFFER_SIZE);

		} else {
			fread(buffer, 1, bytesToSend, in);
			do {
				printf("Sending chunk %d...\n", count);
				bytesSent = sendto(sock, buffer, bytesToSend, 0, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
			} while (bytesSent != bytesToSend);
		}
		// Get fromSize
		fromSize = sizeof(fromAddress);

		// Set bytesToReceive
		bytesToReceive = sizeof(char);

		// Receive acknowledgement
		if ((bytesReceived = recvfrom(sock, &ack, bytesToReceive, 0, (struct sockaddr *) &fromAddress, &fromSize)) != bytesToReceive)
			DieWithError("recvfrom() failed for fileSize ACK");
		printf("Received ACK for Chunk %d ...\n", count);

		// Decrement the bytesToSend
		bytesToSend -= bytesSent;

		// Increment the count
		++count;
	}

	printf("Sent file to server...\n");

	// Close the file
	fclose(in);

	printf("Waiting for response from server ...\n");

	// TODO TODO TODO RECEIVE A RESPONSE FROM THE SERVER
	// Get fromSize
	fromSize = sizeof(fromAddress);

	// Set bytesToReceive
	bytesToReceive = sizeof(char);

	// Receive acknowledgement
	if ((bytesReceived = recvfrom(sock, &serverResponse, bytesToReceive, 0, (struct sockaddr *) &fromAddress, &fromSize)) != bytesToReceive)
		DieWithError("recvfrom() failed for server response");
	printf("Received response from server...\n");

	if (serverResponse = 1)
		printf("Successful\n");
	else
		printf("Format error\n");

	// Close the socket
	close(sock);
	exit(0);
}
