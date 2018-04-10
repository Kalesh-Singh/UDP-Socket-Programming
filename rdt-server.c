#include <stdio.h>					// for printf() and fprintf() 
#include <sys/socket.h>				// for socket(), connect(), sendto(), and recvfrom()
#include <arpa/inet.h>				// for sockaddr_in and inet_addr()
#include <stdlib.h>					// for atoi()
#include <string.h> 				// for memset()
#include <unistd.h>					// for close()
#include <errno.h>					// for errno, EINTR
#include <signal.h>					// for sigaction()
#include "rdt-server-helper.h"		// Helper functions for server


int main(int argc, char* argv[]) {
	int sock;								// Socket
	struct sockaddr_in serverAddress;		// Local address
	struct sockaddr_in clientAddress;		// Client Address
	unsigned int clientAddrLen;				// Length of incoming message
	struct sigaction myAction;			// For setting signal handler
	char buffer[BUFFER_SIZE];				// Buffer to send data
	unsigned short serverPort;				// Server port
	// unsigned long bytesToReceive;			// Number of bytes to receive from client
	// unsigned long bytesReceived;			// Number of bytes received
	unsigned long fileSize;					// Size of file to receive
	char writeStatus;				// Send the result of the operation back to the client
	unsigned char receivedACK;
	unsigned char positiveACK = 1;
	// unsigned long bytesToSend;				// Number of bytes to send to the client
	// unsigned long bytesSent;				// Number of bytes sent
	unsigned short optionsSize;	
	unsigned char toFormat;
	char* toName;
	unsigned char toNameSize;
	float lossProbability;
	int randomSeed;	
	unsigned long remainingBytes;

	// Parse Command Line Arguments
	ParseCommandLineArguments(argc, argv, &serverPort, &lossProbability, &randomSeed);
	printf("Server Port = %d\n", serverPort);
	printf("Loss Probability = %0.2f\n", lossProbability);
	printf("Random Seed = %d\n", randomSeed);


	// Create socket for sending/receiving datagrams
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	// Set the signal handler for alarm signal
	myAction.sa_handler = CatchAlarm;

	if (sigfillset(&myAction.sa_mask) < 0) 		// Block everything in handler
		DieWithError("sigfillset() failed");
	myAction.sa_flags = 0;

	if (sigaction(SIGALRM, &myAction, 0) < 0)
		DieWithError("sigaction() failed for SIGALRM");

	// Cosntruct local address structure
	memset(&serverAddress, 0, sizeof(serverAddress));		// Zero out structure 
	serverAddress.sin_family = AF_INET;						// Internet address family
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);		// Any incoming interface 
	serverAddress.sin_port = htons(serverPort);				// Local port

	// Bind to the local address
	if (bind(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
		DieWithError("bind() failed");

	while (1) { 		// Run forever
		// Set the size of the in-out parameter
		clientAddrLen = sizeof(clientAddress);

		// Receive options size and send ACK
		// NOTE: DO NOT SET A TIME OUT ON THIS FIRST recvfrom()
		// Receive optionsSize
		printf("Waiting for data from a client...\n");
/*
		if ((bytesReceived = recvfrom(sock, &optionsSize, sizeof(optionsSize), 0, (struct sockaddr *) &clientAddress, &clientAddrLen)) != sizeof(optionsSize))

*/
		if ((bytesReceived = recvfrom(sock, recvPacketBuffer, sizeof(char) + sizeof(optionsSize), 0, (struct sockaddr *) &clientAddress, &clientAddrLen)) != sizeof(char) + sizeof(optionsSize))	
			DieWithError("recvform failed() for optionsSize");
		printf("Received optionsSize from client...\n");
		extractPacket(recvPacketBuffer, &seqNum, &optionsSize, sizeof(optionsSize));

		// seqNum = (seqNum + 1) % 2;

		// Send Positive ACK and wait for Options Packet from server
		printf("Sending ACK for optionsSize...\n");
		send_wait(sock, lossProbability, randomSeed, &clientAddress, clientAddrLen, &clientAddress, clientAddrLen, &positiveACK, sizeof(positiveACK), buffer, optionsSize);
		printf("Received Options Package ...\n");

		// Parse the options
		int optionsLen = 0;
		memcpy(&toFormat, buffer + optionsLen, sizeof(toFormat));
		optionsLen += sizeof(toFormat);
		printf("To Format = %d\n", toFormat);
		memcpy(&toNameSize, buffer + optionsLen, sizeof(toNameSize));
		optionsLen += sizeof(toNameSize);
		printf("To Name Size = %d\n", toNameSize);	
		char temp[toNameSize];
		memcpy(temp, buffer + optionsLen, toNameSize);
		toName = temp;
		optionsLen += toNameSize;
		printf("To Name = %s\n", toName);
		memcpy(&fileSize, buffer + optionsLen, sizeof(fileSize));
		printf("File Size = %lu\n", fileSize);

		// Check that options was correctly received
		optionsLen += sizeof(fileSize);
		if (optionsLen != optionsSize) {
			printf("Options Len = %d\n", optionsLen);
			printf("Options Size = %d\n", optionsSize);
			printf("Options not correctly received\n");
			exit(1);
		}

		// Create IN file to be passed to practice project then delete it 
		FILE* tempIn = fopen(".tempFile", "wb");
		if (tempIn == NULL) {
			perror("Failed to open file");
			return -1;
		}
		
		printf("Receiving file from client ...\n");


		// Set bytesToReceive
		remainingBytes = fileSize;

		int count = 1;
		// Receive the file from the 
		while (remainingBytes > 0) {
			if (remainingBytes > BUFFER_SIZE) {
				if (count == 1) 
					printf("Sending ACK for Options Packet ...\n");
				else 
					printf("Sending ACK File Chunk  %d...\n", count);

				send_wait(sock, lossProbability, randomSeed, &clientAddress, clientAddrLen, &clientAddress, clientAddrLen, &positiveACK, sizeof(positiveACK), buffer, BUFFER_SIZE);
				printf("Received File Chunk %d from client ...\n", ++count);
			} else {
				if (count == 1) 
					printf("Sending ACK for Options Packet ...\n");
				else 
					printf("Sending ACK File Chunk  %d...\n", count);

				send_wait(sock, lossProbability, randomSeed, &clientAddress, clientAddrLen, &clientAddress, clientAddrLen, &positiveACK, sizeof(positiveACK), buffer, remainingBytes);
				printf("Received File Chunk %d from client ...\n", ++count);
			}
			fwrite(buffer, bytesReceived, 1, tempIn);
	
			// Decrement bytesToReceive
			remainingBytes += sizeof(char);
			remainingBytes -= bytesReceived; 
		}
		
		// Close the file
		fclose(tempIn);

		writeStatus = 0;

		// TODO: Send ACK for last file chunk and wait for ACK
		printf("Sending ACK File Chunk  %d...\n", count);
		send_wait(sock, lossProbability, randomSeed, &clientAddress, clientAddrLen, &clientAddress, clientAddrLen, &positiveACK, sizeof(positiveACK), &receivedACK, sizeof(receivedACK));
		printf("Received FILE COMPLETE from client ...\n");

		printf("Received file from client...\n");
		
		printf("Handling data received from: %s\n", inet_ntoa(clientAddress.sin_addr));

		// TODO TODO TODO PROCESS THE DATA RECEIVED
		writeStatus = 0;		//TODO: STOP Hardcoding

		

		// Send Respose for some time no ACK will be sent by client
		int secondsLeft = 10;
		printf("Sending response to client...\n");

		// TODO: Make the packet
		unsigned long packetLen = makePacket(sendPacketBuffer, &seqNum, &writeStatus, sizeof(writeStatus));

		while (secondsLeft > 0) {
/*
			if ((bytesSent = lossy_sendto(lossProbability, randomSeed, sock, &writeStatus, sizeof(writeStatus), (struct sockaddr *) &clientAddress, clientAddrLen)) != sizeof(writeStatus))
					DieWithError("lossy_sendto() sent a different number of bytes than expected");
*/
			if ((bytesSent = lossy_sendto(lossProbability, randomSeed, sock, sendPacketBuffer, packetLen, (struct sockaddr *) &clientAddress, clientAddrLen)) != packetLen)
					DieWithError("lossy_sendto() sent a different number of bytes than expected");
			--secondsLeft;		
		}
		printf("Sent Response to Client...\n");

	}


	// NOT REACHED
	return 0;
}
