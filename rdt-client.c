#include <stdio.h>					// for print() and fprintf()
#include <sys/socket.h>				// for socket(), connect(), sendto(), and recvfrom()
#include <arpa/inet.h>				// for sockaddr_in and inet_addr()
#include <stdlib.h>					// for atoi()
#include <string.h>					// for memset()
#include <unistd.h>					// for close()
#include <errno.h>					// for errno, EINTR
#include <signal.h> 				// for sigaction()
#include "rdt-client-helper.h"		// Helper functions for client
#include "sendlib.h"				// for lossy_sendto()

#define BUFFER_SIZE 1000			// Buffer size
#define TIMEOUT_SECS 2				// Timeout seconds
#define MAX_TRIES 5					// Max tries before terminating (Peer inactive)
						

int main(int argc, char* argv[]) {
	int sock;							// Socket descriptor
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
	unsigned long bytesToSend;			// Number of bytes to send to the server
	unsigned long bytesSent;			// Number of bytes sent
	unsigned char receivedACK;
	unsigned char toSendACK;
	char serverResponse;				// Server response (Success or Format error)
	unsigned long bytesToReceive;		// Number of bytes to receive from server
	unsigned long bytesReceived;		// Number of bytes received
	unsigned long fileSize;

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

	// Send the file size to the server
	printf("Sending file size to server...\n");

	// Set bytesToSend
	bytesToSend = sizeof(fileSize);

	// Send the data 
	if ((bytesSent = lossy_sendto(lossProbability, randomSeed, sock, &fileSize, bytesToSend, (struct sockaddr *) &serverAddress, sizeof(serverAddress))) != bytesToSend)
		DieWithError("lossy_sendto() sent a different number of bytes than expected");
	printf("Sent file size...\n");

	// Get a response (ACK)
	
	// Get fromSize
	fromSize = sizeof(fromAddress);

	// Reset received ACK to 0
	receivedACK = 0;

	// Set bytesToReceive
	bytesToReceive = sizeof(receivedACK);

	// Reset tries
	tries = 0;

	// Set the Timeout
	alarm(TIMEOUT_SECS);
	
	while (((bytesReceived = recvfrom(sock, &receivedACK, bytesToReceive, 0, (struct sockaddr *) &fromAddress, &fromSize)) < 0) || (receivedACK != 1)) {
		if (errno == EINTR)	{			// Alarm went off
			if (tries < MAX_TRIES) {	// Incremented by signal handler
				printf("recvfrom() timed out, %d more tries ...\n", MAX_TRIES - tries);
				
				// Resend the data
				if ((bytesSent = lossy_sendto(lossProbability, randomSeed, sock, &fileSize, bytesToSend, (struct sockaddr *) &serverAddress, sizeof(serverAddress))) != bytesToSend)
					DieWithError("lossy_sendto() sent a different number of bytes than expected");

				// Restart the timer
				alarm(TIMEOUT_SECS);
			} else
				DieWithError("No Response");
		} else
			DieWithError("recvfrom() failed");
	}

	// recvfrom() got something -- cancel the timeout
	alarm(0);
	printf("Received fileSize ACK...\n");

	// Send the file to server
	printf("Sending FILE to server ...\n");

	// Set bytesToSend
	bytesToSend = fileSize;

	// Send the file in chucks, which can be as large as the buffer size
	int count = 1;
	while (bytesToSend > 0) {
		// Read data into buffer and send the data
		if (bytesToSend > BUFFER_SIZE) {
			fread(buffer, 1, BUFFER_SIZE, in);
			if ((bytesSent = lossy_sendto(lossProbability, randomSeed, sock, buffer, BUFFER_SIZE, (struct sockaddr *) &serverAddress, sizeof(serverAddress))) != BUFFER_SIZE)
				DieWithError("lossy_sendto() sent a different number of bytes than expected");
			printf("Sent file chunk %d...\n", count);
		} else {
			fread(buffer, 1, bytesToSend, in);
			if ((bytesSent = lossy_sendto(lossProbability, randomSeed, sock, buffer, bytesToSend, (struct sockaddr *) &serverAddress, sizeof(serverAddress))) != bytesToSend)
				DieWithError("lossy_sendto() sent a different number of bytes than expected");
			printf("Sent file chunk %d...\n", count);
		}
		// Get fromSize
		fromSize = sizeof(fromAddress);

		// Reset received ACK to 0
		receivedACK = 0;

		// Set bytesToReceive
		bytesToReceive = sizeof(receivedACK);

		// Reset tries
		tries = 0;

		// Set the Timeout
		alarm(TIMEOUT_SECS);

		while (((bytesReceived = recvfrom(sock, &receivedACK, bytesToReceive, 0, (struct sockaddr *) &fromAddress, &fromSize)) < 0) || (receivedACK != 1)) {
			if (errno == EINTR)	{			// Alarm went off
				if (tries < MAX_TRIES) {	// Incremented by signal handler
					printf("recvfrom() timed out, %d more tries ...\n", MAX_TRIES - tries);
				
					// Resend the data
					if (bytesToSend > BUFFER_SIZE) {
						if ((bytesSent = lossy_sendto(lossProbability, randomSeed, sock, buffer, BUFFER_SIZE, (struct sockaddr *) &serverAddress, sizeof(serverAddress))) != BUFFER_SIZE)
							DieWithError("lossy_sendto() sent a different number of bytes than expected");
						printf("Sent file chunk %d...\n", count);
					} else {
						if ((bytesSent = lossy_sendto(lossProbability, randomSeed, sock, buffer, bytesToSend, (struct sockaddr *) &serverAddress, sizeof(serverAddress))) != bytesToSend)
							DieWithError("lossy_sendto() sent a different number of bytes than expected");
						printf("Sent file chunk %d...\n", count);
					}

					// Restart the timer
					alarm(TIMEOUT_SECS);
				} else
					DieWithError("No Response");
			} else
				DieWithError("recvfrom() failed");
		}

		// recvfrom() got something -- cancel the timeout
		alarm(0);
		printf("Received fileSize ACK...\n");

		// Decrement the bytesToSend
		bytesToSend -= bytesSent;

		// Increment the count
		++count;
	}

	printf("Sent file to server...\n");

	printf("Waiting for response from server ...\n");

	// Get fromSize
	fromSize = sizeof(fromAddress);

	// Set bytesToReceive
	bytesToReceive = sizeof(serverResponse);

	// Reset serverResponse
	serverResponse = -1;

	// Set the Timeout
	alarm(TIMEOUT_SECS * 5);

	while (((bytesReceived = recvfrom(sock, &serverResponse, bytesToReceive, 0, (struct sockaddr *) &fromAddress, &fromSize)) < 0) || (serverResponse != 0 && serverResponse != 1)) {
		if (errno == EINTR)			// Alarm went off
				printf("Request Timed Out...\nClient Terminating ...\n\n");
		 else
			DieWithError("recvfrom() failed");
	}
	// recvfrom() got something -- cancel the timeout
	alarm(0);
	printf("Received Response from server...\n");

	// Process the Response
	if (serverResponse == 0)
		printf("Successful\n");
	else if (serverResponse == 1)
		printf("Format error\n");
	else
		printf("Server Response = %d\n", serverResponse);

	// Close the socket
	close(sock);

	return 0;

}
