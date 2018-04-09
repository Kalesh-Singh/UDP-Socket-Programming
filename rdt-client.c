#include <stdio.h>					// for print() and fprintf()
#include <sys/socket.h>				// for socket(), connect(), sendto(), and recvfrom()
#include <arpa/inet.h>				// for sockaddr_in and inet_addr()
#include <stdlib.h>					// for atoi()
#include <string.h>					// for memset()
#include <unistd.h>					// for close()
#include <errno.h>					// for errno, EINTR
#include <signal.h> 				// for sigaction()
#include "rdt-client-helper.h"		// Helper functions for client
				
int main(int argc, char* argv[]) {
	int sock;						// Socket descriptor
	struct sockaddr_in serverAddress;	// Server address
	struct sockaddr_in fromAddress;		// Source address of the received message
	unsigned short serverPort;			// Server port
	unsigned int fromSize;				// In-out of address size for recvfrom()
	struct sigaction myAction;			// For setting signal handler
	char* serverIP;						// IP address of server
	char* filePath;						// Name (or path of file to send)
	unsigned char toFormat;
	char* toName;
	unsigned char toNameSize;	
	float lossProbability;
	int randomSeed;
	char buffer[BUFFER_SIZE];			// Buffer for receiving messages
	unsigned char positiveACK;
	char serverResponse;				// Server response (Success or Format error)
	unsigned long fileSize;
	unsigned short optionsSize;	
	unsigned char receivedACK;

	//Parse Command Line Arguments
	ParseCommandLineArguments(argc, argv, &serverIP, &serverPort, &filePath, &toFormat, &toName, &toNameSize, &lossProbability, &randomSeed);

	printf("Server IP = %s\n", serverIP);
	printf("Server Port = %d\n", serverPort);
	printf("File Path = %s\n", filePath);
	printf("To Format =  %d\n", toFormat);
	printf("To Name = %s\n", toName);
	//printf("To Name Size = %d\n", toNameSize);
	printf("Loss Probability = %0.2f\n", lossProbability);
	printf("Random Seed = %d\n", randomSeed);
	

	// Create a datagram/UDP socket
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	// Set the signal handler for alarm signal
	myAction.sa_handler = CatchAlarm;

	if (sigfillset(&myAction.sa_mask) < 0) 		// Block everything in handler
		DieWithError("sigfillset() failed");
	myAction.sa_flags = 0;

	if (sigaction(SIGALRM, &myAction, 0) < 0)
		DieWithError("sigaction() failed for SIGALRM");

	// Construct the server address structure
	memset(&serverAddress, 0, sizeof(serverAddress));			// Zero out structure
	serverAddress.sin_family = AF_INET;							// Internet addr family
	serverAddress.sin_addr.s_addr = inet_addr(serverIP);		// Server IP address
	serverAddress.sin_port = htons(serverPort);					// Server Port

	// Try to open the file at file path 
	FILE* in = fopen(filePath, "rb");
	if (in == NULL)
		DieWithError("Failed to open file");

	// Get the size of the file in bytes
	fseek(in, 0L, SEEK_END);
	fileSize = ftell(in);
	printf("File Size = %lu\n", fileSize);
	rewind(in);

	// Get the size of options
	optionsSize = sizeof(toFormat) + sizeof(toNameSize) + toNameSize + sizeof(fileSize);
	printf("Options Size = %d\n", optionsSize);

	// Send options size to server and Wait for server to send ACK for options size
	printf("Sending Options Size to Server...\n");
	send_wait(sock, lossProbability, randomSeed, &serverAddress, sizeof(serverAddress), &fromAddress, sizeof(serverAddress), &optionsSize, sizeof(optionsSize), &receivedACK, sizeof(receivedACK));
	printf("Received ACK for Options Size...\n");

	// Create the options packet
	int optionsLen = 0;
	memcpy(buffer + optionsLen, &toFormat, sizeof(toFormat));			// Place toFormat in the optionsBuffer
	optionsLen += sizeof(toFormat);
	memcpy(buffer + optionsLen, &toNameSize, sizeof(toNameSize));		// Place toNameSize in the optionsBuffer
	optionsLen += sizeof(toNameSize);
	memcpy(buffer + optionsLen, toName, toNameSize);					// Place toName in the optionsBuffer
	optionsLen += toNameSize;
	printf("FIleSize = %lu\n", fileSize);
	memcpy(buffer + optionsLen, &fileSize, sizeof(fileSize));			// Place fileSize in the optionsBuffer

	// Send options packet and wait for ACK
	printf("Sending Options Packet to Server...\n");
	send_wait(sock, lossProbability, randomSeed, &serverAddress, sizeof(serverAddress), &fromAddress, sizeof(serverAddress), buffer, optionsSize, &receivedACK, sizeof(receivedACK));
	printf("Received ACK for Options Packet...\n");

	// Send the file to server
	printf("Sending FILE to server ...\n");
	unsigned long bytesRemaining = fileSize;
	int count = 1;
	
	while (bytesRemaining > 0) {

		if (bytesRemaining > BUFFER_SIZE) {
			fread(buffer, 1, BUFFER_SIZE, in);
			printf("Sending file chunk %d...\n", count);
			send_wait(sock, lossProbability, randomSeed, &serverAddress, sizeof(serverAddress), &fromAddress, sizeof(serverAddress), buffer, BUFFER_SIZE, &receivedACK, sizeof(receivedACK));
			printf("Received ACK file chunk %d...\n", count);
		} else {
			fread(buffer, 1, bytesRemaining, in);
			printf("Sending file chunk %d...\n", count);
			send_wait(sock, lossProbability, randomSeed, &serverAddress, sizeof(serverAddress), &fromAddress, sizeof(serverAddress), buffer, bytesRemaining, &receivedACK, sizeof(receivedACK));
			printf("Received ACK file chunk %d...\n", count);
		}
		
		// Decrement the bytesRemaining
		bytesRemaining -= bytesSent;

		// Increment the count
		++count;
	}
	printf("Sending FILE COMPLETE to server...\n");
	send_wait(sock, lossProbability, randomSeed, &serverAddress, sizeof(serverAddress), &fromAddress, sizeof(serverAddress), &positiveACK, sizeof(positiveACK), &serverResponse, sizeof(serverResponse));
	printf("Received response from server ...\n");

	// Process the Response
	if (serverResponse < 0)
		printf("Format error\n");
		
	else
		printf("Successful\n");

	sleep(1);
	// Close the socket
	close(sock);

	return 0;
}
