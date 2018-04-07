#include <stdio.h>					// for printf() and fprintf() 
#include <sys/socket.h>				// for socket(), connect(), sendto(), and recvfrom()
#include <arpa/inet.h>				// for sockaddr_in and inet_addr()
#include <stdlib.h>					// for atoi()
#include <string.h> 				// for memset()
#include <unistd.h>					// for close()

#define BUFFER_SIZE 1000			// Buffer size

void DieWithError(char* errorMessage) {		
	// Error handling function
	perror(errorMessage);
	exit(1);
}

int main(int argc, char* argv[]) {
	int sock;								// Socket
	struct sockaddr_in serverAddress;		// Local address
	struct sockaddr_in clientAddress;		// Client Address
	unsigned int clientAddrLen;				// Length of incoming message
	char buffer[BUFFER_SIZE];				// Buffer to send data
	unsigned short serverPort;				// Server port
	unsigned long bytesToReceive;			// Number of bytes to receive from client
	unsigned long bytesReceived;			// Number of bytes received
	unsigned long fileSize;					// Size of file to receive
	unsigned char writeStatus;				// Send the result of the operation back to the client
	unsigned char ack;
	unsigned char positiveAck = 1;
	unsigned long bytesToSend;				// Number of bytes to send to the client
	unsigned long bytesSent;				// Number of bytes sent

	if (argc != 2) {						// Test for correct number of parameters
		fprintf(stderr, "Usage: %s <UDP SERVER PORT>\n", argv[0]);
		exit(1);
	}

	serverPort = atoi(argv[1]);		// First arg: local port

	// Create socket for sending/receiving datagrams
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	// Cosntruct local address structure
	memset(&serverAddress, 0, sizeof(serverAddress));		// Zero out structure 
	serverAddress.sin_family = AF_INET;						// Internet address family
	//serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);		// Any incoming interface 
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(serverPort);				// Local port

	// Bind to the local address
	if (bind(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
		DieWithError("bind() failed");

	while (1) { 		// Run forever
		// Set the size of the in-out parameter
		clientAddrLen = sizeof(clientAddress);

		printf("Waiting for fileSize from client ...\n");
		
		// Set bytesToReceive
		bytesToReceive = sizeof(long);

		// TODO: DO NOT SET A TIME OU ON THIS INITIAL RECEIVE FROM
		// Receive fileSize
		if ((bytesReceived = recvfrom(sock, &fileSize, bytesToReceive, 0, (struct sockaddr *) &clientAddress, &clientAddrLen)) != bytesToReceive)
			DieWithError("recvform failed() for fileSize");
		printf("Received fileSize from client...\n");
		
		// Set bytesToSend
		bytesToSend = sizeof(char);

/*
if ((bytesSent = sendto(sock, buffer, bytesToSend, 0, (struct sockaddr *) &serverAddress, sizeof(serverAddress))) != bytesToSend)
				DieWithError("sendto() sent a different number of bytes than expected");
*/

		// Send positive acknowledgement
		if ((bytesSent = sendto(sock, &positiveAck, bytesToSend, 0, (struct sockaddr *) &clientAddress, sizeof(clientAddress))) != bytesToSend)
			DieWithError("sendto() sent a different number of bytes than expected");
		printf("Sent ACK for fileSize...\n");

		// Create IN file to be passed to practice project then delete it 
		FILE* tempIn = fopen(".tempFile", "wb");
		if (tempIn == NULL) {
			perror("Failed to open file");
			return -1;
		}
		
		printf("Receiving file from client ...\n");

		// Set bytesToReceive
		bytesToReceive = fileSize;

		int count = 1;
		// Receive the file from the 
		while (bytesToReceive > 0) {
			if (bytesToReceive > BUFFER_SIZE) {
				if((bytesReceived = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &clientAddress, &clientAddrLen)) != BUFFER_SIZE) {
					printf("Bytes Received = %lu\n", bytesReceived);
					printf("Bytes To Receive = %lu\n", bytesToReceive);
					DieWithError("recvfrom() failed");
				}
			} else {
				if((bytesReceived = recvfrom(sock, buffer, bytesToReceive, 0, (struct sockaddr *) &clientAddress, &clientAddrLen)) != bytesToReceive) {
					printf("Bytes Received = %lu\n", bytesReceived);
					printf("Bytes To Receive = %lu\n", bytesToReceive);
					DieWithError("recvfrom() failed");
				}
			}
			fwrite(buffer, bytesReceived, 1, tempIn);
			printf("Received Chunk %d...\n", count);

			// Set bytesToSend
			bytesToSend = sizeof(char);

			// Send positive acknowledgement
			if ((bytesSent = sendto(sock, &positiveAck, bytesToSend, 0, (struct sockaddr *) &clientAddress, sizeof(clientAddress))) != bytesToSend)
				DieWithError("sendto() sent a different number of bytes than expected");
			printf("Sent ACK for Chunk %d ...\n", count);

			// Decrement bytesToReceive
			bytesToReceive -= bytesReceived;

			// Increment the count
			++count;
		}
		
		// Close the file
		fclose(tempIn);

		printf("Received file from client...\n");
		
		printf("Handling data received from: %s\n", inet_ntoa(clientAddress.sin_addr));

		// TODO TODO TODO PROCESS THE DATA RECEIVED
		writeStatus = 0;		//TODO: STOP Hardcoding

		printf("Sending response to client...\n");

		// TODO TODO : Send Respose for some time no ACK will be sent by client
/*
		e.g. while (alarm()) {
			sendServerResponse()

		// TODO: NOTE: SERVER RESPONSE MUST BE 0 OR 1 
		// WILL HAVE TO CHANGE THE RETURN VALUES OF writeUnitsFunction
		}
*/

		// Set the bytesToSend
		bytesToSend = sizeof(char);

		if ((bytesSent = sendto(sock, &writeStatus, bytesToSend, 0, (struct sockaddr *) &clientAddress, sizeof(clientAddress))) != bytesToSend)
			DieWithError("sendto() sent a different number of bytes than expected");
		printf("Sent response to client...\n");

		// Set bytesToReceive
		bytesToReceive = sizeof(char);


	}

	// NOT REACHED
}
